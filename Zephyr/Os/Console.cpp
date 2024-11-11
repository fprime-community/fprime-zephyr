// ======================================================================
// \title Os/Zephyr/Console.cpp
// \brief Zephyr implementation for Os::Console
// ======================================================================
#include <Zephyr/Os/Console.hpp>
#include <limits>
#include <cstdio>

namespace Os {
namespace Zephyr {

void ZephyrConsole::writeMessage(const CHAR *message, const FwSizeType size) {
    if (message != nullptr) {
        (void) printk("%s", message);
    }
}

ConsoleHandle* ZephyrConsole::getHandle() {
    return &this->m_handle;
}


} // namespace Stub
} // namespace Os
