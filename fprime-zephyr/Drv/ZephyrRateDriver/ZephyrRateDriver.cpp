// ======================================================================
// \title  ZephyrRateDriver.cpp
// \author ethanchee
// \brief  cpp file for ZephyrRateDriver component implementation class
// ======================================================================

#include "fprime-zephyr/Drv/ZephyrRateDriver/ZephyrRateDriver.hpp"
// #include <FpConfig.hpp>
#include <Fw/Logger/Logger.hpp>
#include <Fw/FPrimeBasicTypes.hpp>

namespace Zephyr{


    // ----------------------------------------------------------------------
    // Construction, initialization, and destruction
    // ----------------------------------------------------------------------

    ZephyrRateDriver ::
        ZephyrRateDriver(
            const char *const compName) : ZephyrRateDriverComponentBase(compName)
    {
    }

    ZephyrRateDriver ::
        ~ZephyrRateDriver()
    {
    }

    void ZephyrRateDriver::configure(U32 intervalMs)
    {
        this->m_interval = intervalMs;
    }

    void ZephyrRateDriver::start()
    {
        U32 microseconds = this->m_interval * 1000;
        Fw::Logger::log("Starting base rate group clock with period of %" PRIu32 " microseconds\n", microseconds);
        k_timer_init(&this->m_timer, NULL, NULL);

        /* start periodic timer */
        k_timer_start(&this->m_timer, K_USEC(microseconds), K_USEC(microseconds));
    }

    void ZephyrRateDriver::stop()
    {
        k_timer_stop(&this->m_timer);
    }

    void ZephyrRateDriver::cycle()
    {
        // Cycling the rate group involves waiting on the timer to expire
        // then driving the rate groups
        if (k_timer_status_sync(&this->m_timer) > 0)
        {
            Os::RawTime now;
            now.now();

            // Check if it is time to run the group
            if (this->isConnected_CycleOut_OutputPort(0))
            {
                this->CycleOut_out(0, now);
            }
        }

    }


} // end namespace Zephyr
