// Stub of the FPP-generated component base: records port calls and serves buffers from a pool
#ifndef ZEPHYR_UART_DRIVER_COMPONENT_AC_HPP
#define ZEPHYR_UART_DRIVER_COMPONENT_AC_HPP
#include <Fw/Buffer.hpp>
#include <Fw/FPrimeBasicTypes.hpp>

#include <cstring>
#include <vector>

namespace Drv {
struct ByteStreamStatus {
    enum T { OP_OK, SEND_RETRY, RECV_NO_DATA, OTHER_ERROR };
    ByteStreamStatus(T t = OP_OK) : e(t) {}
    bool operator==(T t) const { return e == t; }
    T e;
};
}  // namespace Drv

namespace Zephyr {

class ZephyrUartDriverComponentBase {
  public:
    static constexpr FwSizeType POOL_SLOT_SIZE = 1024;
    static constexpr FwSizeType POOL_SLOTS = 16;

    explicit ZephyrUartDriverComponentBase(const char* const compName) : m_compName(compName) { resetHarness(); }
    virtual ~ZephyrUartDriverComponentBase() {}

    // --- test harness state ---
    struct Recv { std::vector<U8> data; Drv::ByteStreamStatus::T status; };
    std::vector<Recv> h_recv;
    std::vector<U8> h_recvStream;       // concatenation of all delivered bytes
    std::vector<FwSizeType> h_allocSizes;
    int h_deallocCount = 0;
    int h_emptyDeallocCount = 0;
    int h_readyCount = 0;
    bool h_readyConnected = true;
    bool h_allocFail = false;           // allocator returns an empty buffer
    int h_outstanding = 0;              // buffers allocated and not yet returned
    std::vector<U32> h_rxOverrunEvents;
    U32 h_tlmRxBytes = 0, h_tlmRxOverrun = 0, h_tlmRxBackpressure = 0, h_tlmRxAllocFail = 0;

    // --- port invocation (the generated base dispatches to the private handlers) ---
    void schedIn_handler_public(FwIndexType portNum, U32 context) { this->schedIn_handler(portNum, context); }
    Drv::ByteStreamStatus send_handler_public(FwIndexType portNum, Fw::Buffer& buffer) { return this->send_handler(portNum, buffer); }
    void recvReturnIn_handler_public(FwIndexType portNum, Fw::Buffer& buffer) { this->recvReturnIn_handler(portNum, buffer); }

    void resetHarness() {
        h_recv.clear(); h_recvStream.clear(); h_allocSizes.clear();
        h_deallocCount = h_emptyDeallocCount = h_readyCount = h_outstanding = 0;
        h_readyConnected = true; h_allocFail = false;
        h_rxOverrunEvents.clear();
        h_tlmRxBytes = h_tlmRxOverrun = h_tlmRxBackpressure = h_tlmRxAllocFail = 0;
        std::memset(m_used, 0, sizeof(m_used));
    }

  private:
    virtual void schedIn_handler(FwIndexType portNum, U32 context) = 0;
    virtual Drv::ByteStreamStatus send_handler(FwIndexType portNum, Fw::Buffer& sendBuffer) = 0;
    virtual void recvReturnIn_handler(FwIndexType portNum, Fw::Buffer& returnBuffer) = 0;

  protected:
    // --- output ports ---
    Fw::Buffer allocate_out(FwIndexType, FwSizeType size) {
        h_allocSizes.push_back(size);
        if (h_allocFail || size > POOL_SLOT_SIZE) {
            return Fw::Buffer();
        }
        for (FwSizeType i = 0; i < POOL_SLOTS; i++) {
            if (!m_used[i]) {
                m_used[i] = true;
                h_outstanding++;
                return Fw::Buffer(m_pool[i], size);
            }
        }
        return Fw::Buffer();
    }
    void deallocate_out(FwIndexType, Fw::Buffer& buffer) {
        h_deallocCount++;
        if (buffer.getData() == nullptr) {
            h_emptyDeallocCount++;
            return;
        }
        release(buffer);
    }
    void recv_out(FwIndexType, Fw::Buffer& buffer, const Drv::ByteStreamStatus& status) {
        Recv r;
        r.data.assign(buffer.getData(), buffer.getData() + buffer.getSize());
        r.status = status.e;
        h_recvStream.insert(h_recvStream.end(), r.data.begin(), r.data.end());
        h_recv.push_back(r);
        release(buffer);  // downstream returns the buffer (recvReturnIn -> deallocate) immediately
    }
    void ready_out(FwIndexType) { h_readyCount++; }
    bool isConnected_ready_OutputPort(FwIndexType) const { return h_readyConnected; }

    // --- events / telemetry ---
    void log_WARNING_LO_RxOverrun(U32 total) { h_rxOverrunEvents.push_back(total); }
    void tlmWrite_RxBytes(U32 v) { h_tlmRxBytes = v; }
    void tlmWrite_RxOverrunCount(U32 v) { h_tlmRxOverrun = v; }
    void tlmWrite_RxBackpressureCount(U32 v) { h_tlmRxBackpressure = v; }
    void tlmWrite_RxAllocFailCount(U32 v) { h_tlmRxAllocFail = v; }

  private:
    void release(Fw::Buffer& buffer) {
        for (FwSizeType i = 0; i < POOL_SLOTS; i++) {
            if (buffer.getData() == m_pool[i]) {
                m_used[i] = false;
                h_outstanding--;
                return;
            }
        }
    }
    const char* m_compName;
    U8 m_pool[POOL_SLOTS][POOL_SLOT_SIZE];
    bool m_used[POOL_SLOTS];
};

}  // namespace Zephyr
#endif
