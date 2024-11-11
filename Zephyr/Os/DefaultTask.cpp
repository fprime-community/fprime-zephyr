// ======================================================================
// \title Os/Zephyr/DefaultTask.cpp
// \brief sets default Os::Task to test Zephyr implementation via linker
// ======================================================================
#include "Os/Task.hpp"
#include "Zephyr/Os/Task.hpp"
#include "Os/Delegate.hpp"

namespace Os {
    TaskInterface* TaskInterface::getDelegate(TaskHandleStorage& aligned_new_memory) {
        return Os::Delegate::makeDelegate<TaskInterface, Os::Zephyr::ZephyrTask>(aligned_new_memory);
    }

}
