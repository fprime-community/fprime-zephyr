// ======================================================================
// \title  ZephyrTimeImpl.cpp
// \author ethanchee
// \brief  cpp file for ZephyrTime component implementation class
// ======================================================================

#include <Zephyr/ZephyrTime/ZephyrTimeImpl.hpp>
#include <Fw/Time/Time.hpp>

namespace Svc {

    ZephyrTimeImpl::ZephyrTimeImpl(const char* name) : TimeComponentBase(name) {} 
    ZephyrTimeImpl::~ZephyrTimeImpl() {}

    void ZephyrTimeImpl::timeGetPort_handler(
        NATIVE_INT_TYPE portNum, /*!< The port number*/
        Fw::Time &time /*!< The time to set */
    ) {
        struct timespec stime;
        (void)clock_gettime(CLOCK_REALTIME,&stime);
        time.set(TB_PROC_TIME,0, stime.tv_sec, stime.tv_nsec/1000);
    }

    void ZephyrTimeImpl::init(NATIVE_INT_TYPE instance) {
        TimeComponentBase::init(instance);
    }
}
