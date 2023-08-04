#include <Os/Queue.hpp>
#include <Fw/Types/Assert.hpp>

#include <zephyr/kernel.h>
#include <cstring>

#define ZEPHYR_Q_MAX_MSG_SIZE 300
// U8 is reserved for the message size
#define ZEPHYR_Q_RESERVED (sizeof(U16))
#define ZEPHYR_Q_MAX_USER_MSG_SIZE (ZEPHYR_Q_MAX_MSG_SIZE - ZEPHYR_Q_RESERVED)

// Warnings: 1) Priorities are ignored
//           2) Max message size is ZEPHYR_Q_MAX_MSG_SIZE
//           3) Uses extra memcpy for send/receive

namespace Os {

    Queue::Queue() : m_handle(0) {}

    Queue::QueueStatus Queue::createInternal(const Fw::StringBase &name, NATIVE_INT_TYPE depth, NATIVE_INT_TYPE msgSize) {
        this->m_name = "/Q_";
        this->m_name += name;
        
        if (msgSize > static_cast<NATIVE_INT_TYPE>(ZEPHYR_Q_MAX_USER_MSG_SIZE)) {
            return QUEUE_SIZE_MISMATCH;
        }

        struct k_msgq *msgq = reinterpret_cast<struct k_msgq *>(k_malloc(sizeof(struct k_msgq)));
        if (msgq == nullptr) {
            return QUEUE_UNINITIALIZED;
        }
        
        FW_ASSERT(msgSize + ZEPHYR_Q_RESERVED <= ZEPHYR_Q_MAX_MSG_SIZE, msgSize);
        NATIVE_INT_TYPE ret = k_msgq_alloc_init(msgq, msgSize + ZEPHYR_Q_RESERVED, depth);
        if (ret != 0) {
            return QUEUE_UNINITIALIZED;
        }

        this->m_handle = reinterpret_cast<POINTER_CAST>(msgq);
        Queue::s_numQueues++;

        return QUEUE_OK;
    }

    Queue::~Queue() {
        FW_ASSERT(this->m_handle != 0);
        
        struct k_msgq *msgq = reinterpret_cast<struct k_msgq *>(this->m_handle);
        NATIVE_INT_TYPE ret = k_msgq_cleanup(msgq);
        FW_ASSERT(ret == 0, ret);
        k_free(msgq);
    }

    Queue::QueueStatus Queue::send(const U8* buffer, NATIVE_INT_TYPE size, NATIVE_INT_TYPE priority, QueueBlocking block) {
        if (this->m_handle == 0) {
            return QUEUE_UNINITIALIZED;
        }

        if (buffer == nullptr) {
            return QUEUE_EMPTY_BUFFER;
        }

        if (size < 0 || size > this->getMsgSize()) {
            return QUEUE_SIZE_MISMATCH;
        }

        struct k_msgq *msgq = reinterpret_cast<struct k_msgq *>(this->m_handle);

        U8 tmp_buf[ZEPHYR_Q_MAX_MSG_SIZE];
        *reinterpret_cast<U16 *>(tmp_buf) = static_cast<U16>(size);
        memcpy(&tmp_buf[ZEPHYR_Q_RESERVED], buffer, size);
        NATIVE_INT_TYPE ret = k_msgq_put(msgq, tmp_buf, (block == QUEUE_BLOCKING) ? K_FOREVER : K_NO_WAIT);

        if (ret != 0) {
            switch (block) {
                case QUEUE_BLOCKING:
                    FW_ASSERT(0, ret); // Nonzero return shall not occur for blocking send
                    return QUEUE_SEND_ERROR;
                case QUEUE_NONBLOCKING:
                    FW_ASSERT(ret != -EAGAIN, ret); // EAGAIN means timeout expired, shall not occur
                    return QUEUE_FULL;
                default:
                    FW_ASSERT(0, block);
                    return QUEUE_SEND_ERROR;
            }
        }

        return QUEUE_OK;
    }

    Queue::QueueStatus Queue::receive(U8* buffer, NATIVE_INT_TYPE capacity, NATIVE_INT_TYPE &actualSize, NATIVE_INT_TYPE &priority, QueueBlocking block) {
        if (this->m_handle == 0) {
            return QUEUE_UNINITIALIZED;
        }

        if (buffer == nullptr) {
            return QUEUE_EMPTY_BUFFER;
        }

        if (capacity < this->getMsgSize()) {
            return QUEUE_SIZE_MISMATCH;
        }

        // Priority is not supported in Zephyr
        priority = 0;

        struct k_msgq *msgq = reinterpret_cast<struct k_msgq *>(this->m_handle);
        U8 tmp_buf[ZEPHYR_Q_MAX_MSG_SIZE];
        NATIVE_INT_TYPE ret = k_msgq_get(msgq, tmp_buf, (block == QUEUE_BLOCKING) ? K_FOREVER : K_NO_WAIT);
        
        if (ret != 0) {
            switch (block) {
                case QUEUE_BLOCKING:
                    FW_ASSERT(0, ret); // Nonzero return shall not occur for blocking receive
                    return QUEUE_RECEIVE_ERROR;
                case QUEUE_NONBLOCKING:
                    FW_ASSERT(ret != -EAGAIN, ret); // EAGAIN means timeout expired, shall not occur
                    return QUEUE_NO_MORE_MSGS;
                default:
                    FW_ASSERT(0, block);
                    return QUEUE_RECEIVE_ERROR;
            }
        }

        actualSize = *reinterpret_cast<U16 *>(tmp_buf);
        memcpy(buffer, &tmp_buf[ZEPHYR_Q_RESERVED], actualSize);
        return QUEUE_OK;
    }

    NATIVE_INT_TYPE Queue::getNumMsgs() const {
        FW_ASSERT(this->m_handle != 0, this->m_handle);

        struct k_msgq *msgq = reinterpret_cast<struct k_msgq *>(this->m_handle);
        return static_cast<U32>(msgq->used_msgs);
    }

    // getMaxMsgs() and getQueueSize() are confusingly named.
    NATIVE_INT_TYPE Queue::getMaxMsgs() const {
        FW_ASSERT(0);
        return 0;
    }

    NATIVE_INT_TYPE Queue::getQueueSize() const {
        FW_ASSERT(this->m_handle != 0, this->m_handle);

        struct k_msgq *msgq = reinterpret_cast<struct k_msgq *>(this->m_handle);
        return static_cast<U32>(msgq->max_msgs);
    }

    NATIVE_INT_TYPE Queue::getMsgSize() const {
        FW_ASSERT(this->m_handle != 0, this->m_handle);

        struct k_msgq *msgq = reinterpret_cast<struct k_msgq *>(this->m_handle);
        FW_ASSERT(msgq->msg_size >= ZEPHYR_Q_RESERVED, msgq->msg_size);
        return static_cast<U32>(msgq->msg_size - ZEPHYR_Q_RESERVED);
    }

}