// ======================================================================
// \title Os/Posix/DefaultTask.cpp
// \brief sets default Os::Task to posix implementation via linker
// ======================================================================
#include "Os/Task.hpp"
#include "Os/Zepyhr/Task.hpp"
#include "Os/Delegate.hpp"

namespace Os {

    TaskInterface* TaskInterface::getDelegate(TaskHandleStorage& aligned_new_memory) {
        return Os::Delegate::makeDelegate<TaskInterface, Os::Zephyr::Task>(aligned_new_memory);
    }

}