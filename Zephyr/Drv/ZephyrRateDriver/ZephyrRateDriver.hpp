// ======================================================================
// \title  ZephyrRateDriver.hpp
// \author ethanchee
// \brief  hpp file for ZephyrRateDriver component implementation class
// ======================================================================

#ifndef ZephyrRateDriver_HPP
#define ZephyrRateDriver_HPP

#include "Zephyr/Drv/ZephyrRateDriver/ZephyrRateDriverComponentAc.hpp"
#include <Svc/Cycle/TimerVal.hpp>
#include <zephyr/kernel.h>

namespace Zephyr {

  class ZephyrRateDriver :
    public ZephyrRateDriverComponentBase
  {

    public:

        // ----------------------------------------------------------------------
        // Construction, initialization, and destruction
        // ----------------------------------------------------------------------

        //! Construct object ZephyrRateDriver
        //!
        ZephyrRateDriver(
            const char *const compName /*!< The component name*/
        );

        //! Destroy object ZephyrRateDriver
        //!
        ~ZephyrRateDriver();

        /**
         * Configure this component with the interval time in milliseconds.
         * \param U32 intervalMs: interval to ping in milliseconds
         */
        void configure(U32 intervalMs);

        /**
         * Starts this driver, kicking off the hardware interrupt or the Linux
         * task that virtualizes this driver.
         */
        void start();
        /**
         * Stops the hardware driver, enabling some clean-up.
         */
        void stop();

        void cycle();

        //!< Static callback for task function
        static void s_timer(void *comp);

        //!< Interval of the driver
        U32 m_interval;

    PRIVATE:

        //!< Last time of run
        Svc::TimerVal m_last;
        static ZephyrRateDriver* s_driver;
        struct k_timer s_itimer;

    };

} // end namespace Zephyr

#endif
