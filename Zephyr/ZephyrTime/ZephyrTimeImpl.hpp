// ======================================================================
// \title  ZephyrTimeImpl.hpp
// \author ethanchee
// \brief  hpp file for ZephyrTime component implementation class
// ======================================================================

#ifndef ZEPHYRTIMEIMPL_HPP_
#define ZEPHYRTIMEIMPL_HPP_

#include <Svc/Time/TimeComponentAc.hpp>
#include <zephyr/posix/time.h>

namespace Svc {

class ZephyrTimeImpl: public TimeComponentBase {
    public:
        ZephyrTimeImpl(const char* compName);
        virtual ~ZephyrTimeImpl();
        void init(NATIVE_INT_TYPE instance);
    protected:
        void timeGetPort_handler(
                NATIVE_INT_TYPE portNum, /*!< The port number*/
                Fw::Time &time /*!< The U32 cmd argument*/
            );
    private:
};

}

#endif /* ZEPHYRTIMEIMPL_HPP_ */
