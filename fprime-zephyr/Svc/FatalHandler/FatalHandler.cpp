// ======================================================================
// \title  FatalHandlerImpl.cpp
// \author mstarch
// \brief  cpp file for FatalHandler component implementation class
//
// \copyright
// Copyright 2009-2015, by the California Institute of Technology.
// ALL RIGHTS RESERVED.  United States Government Sponsorship
// acknowledged.
//
// ======================================================================

#include <Fw/Logger/Logger.hpp>
#include <fprime-zephyr/Svc/FatalHandler/FatalHandler.hpp>
#include <Fw/FPrimeBasicTypes.hpp>
#include <zephyr/sys/reboot.h>

namespace Zephyr {

  // ----------------------------------------------------------------------
  // Construction, initialization, and destruction
  // ----------------------------------------------------------------------

  FatalHandler ::
    FatalHandler(
        const char *const compName
    ) : FatalHandlerComponentBase(compName)
  {

  }

  FatalHandler ::
    ~FatalHandler()
  {

  }

  void FatalHandler::reboot() {

    // Use Zephyr to reboot the system
    sys_reboot(SYS_REBOOT_COLD);
    while(1) {}
  }

  void FatalHandler::FatalReceive_handler(
        const FwIndexType portNum,
        FwEventIdType Id) {
      Fw::Logger::log("FATAL %" PRI_FwEventIdType "handled.\n",Id);
      Os::Task::delay(Fw::TimeInterval(0, 1000)); // Delay to allow log to be processed
      this->reboot(); // Reboot the system
    }

} // end namespace Svc
