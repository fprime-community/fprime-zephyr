// ======================================================================
// \title  ZephyrRateDriver.hpp
// \author ethanchee
// \brief  hpp file for ZephyrRateDriver component implementation class
// ======================================================================

#ifndef ZephyrRateDriver_HPP
#define ZephyrRateDriver_HPP

#include "fprime-zephyr/Drv/ZephyrRateDriver/ZephyrRateDriverComponentAc.hpp"
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


        //!< Call from main loop to cycle the rate group
        void cycle();

        //!< Interval of the driver
        U32 m_interval;

    private:
        //!< Last time of run
        struct k_timer m_timer;

    };

} // end namespace Zephyr

#endif
