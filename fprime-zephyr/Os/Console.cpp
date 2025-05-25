// ======================================================================
// \title fprime-zephyr/Os/Console.cpp
// \brief Zephyr implementation for Os::Console
// ======================================================================
#include <fprime-zephyr/Os/Console.hpp>
#include <Fw/Types/Assert.hpp>
#include <zephyr/kernel.h>
#include <limits>

namespace Os {
namespace Zephyr {
namespace Console {


void ZephyrConsole::writeMessage(const CHAR *message, const FwSizeType size) {
    int message_size = (size >= std::numeric_limits<int>::max()) ? std::numeric_limits<int>::max() : static_cast<int>(size);
    printk("[Os::Console] %.*s", message_size, message);
}

ConsoleHandle* ZephyrConsole::getHandle() {
    return &this->m_handle;
}

} // namespace Console
} // namespace Zephyr
} // namespace Os
