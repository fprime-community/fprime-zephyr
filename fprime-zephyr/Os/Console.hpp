// ======================================================================
// \title Os/Zephyr/Console.hpp
// \brief Zephyr implementation for Os::Console, header and test definitions
// ======================================================================
#include <Os/Console.hpp>
#ifndef OS_Zephyr_Console_HPP
#define OS_Zephyr_Console_HPP

namespace Os {
namespace Zephyr {
namespace Console {

//! ConsoleHandle class definition for Zephyr implementations.
//!
struct ZephyrConsoleHandle : public ConsoleHandle {
};

//! \brief Zephyr implementation of Os::ConsoleInterface
//!
//! Zephyr implementation of `ConsoleInterface` for use as a delegate class handling Zephyr console operations. Zephyr
//! consoles write to either standard out or standard error. The default file descriptor used is standard out. This may
//! be changed by calling `setOutputStream`.
//!
class ZephyrConsole : public ConsoleInterface {
  public:
    //! \brief constructor
    //!
    ZephyrConsole() = default;

    //! \brief copy constructor
    ZephyrConsole(const ZephyrConsole& other) = default;

    //! \brief assignment operator that copies the internal representation
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
    //! File handle for ZephyrFile
    ZephyrConsoleHandle m_handle;
};
} // namespace Console
} // namespace Zephyr
} // namespace Os

#endif // OS_Zephyr_Console_HPP
