// ======================================================================
// \title Os/Zephyr/Console.hpp
// \brief Zephyr implementation for Os::Console, header and test definitions
// ======================================================================
#ifndef OS_ZEPHYR_CONSOLE_HPP
#define OS_ZEPHYR_CONSOLE_HPP
#include <Os/Console.hpp>
#include <zephyr/kernel.h>
namespace Os {
namespace Zephyr {

//! ConsoleHandle class definition for stub implementations.
//!
struct ZephyrConsoleHandle : public ConsoleHandle {

};

//! \brief implementation of Os::Console that routes to an Zephyr console
//!
class ZephyrConsole : public ConsoleInterface {
  public:
    //! \brief constructor
    //!
    ZephyrConsole() = default;

    //! \brief copy constructor
    ZephyrConsole(const ZephyrConsole& other) = default;

    //! \brief default copy assignment
    ZephyrConsole& operator=(const ZephyrConsole& other) = default;

    //! \brief destructor
    //!
    ~ZephyrConsole() override = default;

    // ------------------------------------
    // Functions overrides
    // ------------------------------------

    //! \brief write message to console
    //!
    //! Write a message to the console with a bounded size. This will use the active file descriptor as the output
    //! destination.
    //!
    //! \param message: raw message to write
    //! \param size: size of the message to write to the console
    void writeMessage(const CHAR *message, const FwSizeType size) override;

    //! \brief returns the raw console handle
    //!
    //! Gets the raw console handle from the implementation. Note: users must include the implementation specific
    //! header to make any real use of this handle. Otherwise it will be as an opaque type.
    //!
    //! \return raw console handle
    //!
    ConsoleHandle *getHandle() override;
  private:
    //! File handle for PosixFile
    ZephyrConsoleHandle m_handle;
};
} // namespace Zephyr
} // namespace Os

#endif // OS_Stub_Console_HPP
