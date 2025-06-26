// ======================================================================
// \title  FatalHandlerImpl.hpp
// \author mstarch
// \brief  hpp file for FatalHandler component implementation class
//
// \copyright
// Copyright 2009-2015, by the California Institute of Technology.
// ALL RIGHTS RESERVED.  United States Government Sponsorship
// acknowledged.
//
// ======================================================================

#ifndef FatalHandler_HPP
#define FatalHandler_HPP

#include "fprime-zephyr/Svc/FatalHandler/FatalHandlerComponentAc.hpp"

namespace Zephyr {

  class FatalHandler final :
    public FatalHandlerComponentBase
  {

    public:

      // ----------------------------------------------------------------------
      // Construction, initialization, and destruction
      // ----------------------------------------------------------------------

      //! Construct object FatalHandler
      //!
      FatalHandler(
          const char *const compName /*!< The component name*/
      );

      //! Destroy object FatalHandler
      //!
      ~FatalHandler();

      //! Reboot the device
      //!
      void reboot();

    private:

      // ----------------------------------------------------------------------
      // Handler implementations for user-defined typed input ports
      // ----------------------------------------------------------------------

      //! Handler implementation for FatalReceive
      //!
      void FatalReceive_handler(
          const FwIndexType portNum, /*!< The port number*/
          FwEventIdType Id /*!< The ID of the FATAL event*/
      );
    };

} // end namespace Svc

#endif
