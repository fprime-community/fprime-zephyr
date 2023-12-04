// ======================================================================
// \title  ZephyrTime.hpp
// \author ethanchee
// \brief  hpp file for ZephyrTime component implementation class
// ======================================================================

#ifndef ZEPHYRTIMEIMPL_HPP_
#define ZEPHYRTIMEIMPL_HPP_

#include <Zephyr/ZephyrTime/ZephyrTimeComponentAc.hpp>
#include <zephyr/posix/time.h>

namespace Zephyr {

class ZephyrTime: public ZephyrTimeComponentBase {
    public:
        ZephyrTime(const char* compName);
        virtual ~ZephyrTime();
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
