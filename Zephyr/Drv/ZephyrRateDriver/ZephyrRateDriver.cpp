// ======================================================================
// \title  ZephyrRateDriver.cpp
// \author ethanchee
// \brief  cpp file for ZephyrRateDriver component implementation class
// ======================================================================

#include <Zephyr/Drv/ZephyrRateDriver/ZephyrRateDriver.hpp>
#include <FpConfig.hpp>
#include <Fw/Logger/Logger.hpp>

namespace Zephyr
{

    ZephyrRateDriver *ZephyrRateDriver::s_driver = NULL;

    // ----------------------------------------------------------------------
    // Construction, initialization, and destruction
    // ----------------------------------------------------------------------

    ZephyrRateDriver ::
        ZephyrRateDriver(
            const char *const compName) : ZephyrRateDriverComponentBase(compName)
    {
        s_driver = this;
    }

    ZephyrRateDriver ::
        ~ZephyrRateDriver()
    {
    }

    void ZephyrRateDriver::configure(U32 intervalMs)
    {
        this->m_interval = intervalMs;
    }

    void ZephyrRateDriver::s_timer(void *comp)
    {
        Svc::TimerVal now;
        now.take();

        ZephyrRateDriver *driver = reinterpret_cast<ZephyrRateDriver *>(comp);
        // Check if it is time to run the group
        if (driver->isConnected_CycleOut_OutputPort(0))
        {
            driver->CycleOut_out(0, now);
        }
        driver->m_last = now;
    }

    void ZephyrRateDriver::start()
    {
        U32 microseconds = this->m_interval * 1000;
        Fw::Logger::logMsg("Starting base rate group clock with period of %" PRIu32 " microseconds", microseconds);
        k_timer_init(&s_itimer, NULL, NULL);

        /* start periodic timer */
        k_timer_start(&s_itimer, K_USEC(microseconds), K_USEC(microseconds));
    }

    void ZephyrRateDriver::stop()
    {
        k_timer_stop(&s_itimer);
    }

    void ZephyrRateDriver::cycle()
    {
        if (k_timer_status_get(&s_itimer) > 0)
        {
            s_timer(s_driver);
        }
    }

} // end namespace Zephyr
