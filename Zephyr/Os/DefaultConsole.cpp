// ======================================================================
// \title Os/Zephyr/DefaultConsole.cpp
// \brief sets default Os::Console to Zephyr implementation via linker
// ======================================================================
#include "Os/Console.hpp"
#include "Zephyr/Os/Console.hpp"
#include "Os/Delegate.hpp"

namespace Os {
ConsoleInterface* ConsoleInterface::getDelegate(ConsoleHandleStorage& aligned_new_memory, const ConsoleInterface* to_copy) {
    return Os::Delegate::makeDelegate<ConsoleInterface, Os::Zephyr::ZephyrConsole>(aligned_new_memory, to_copy);
}
}
