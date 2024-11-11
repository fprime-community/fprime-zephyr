// ======================================================================
// \title Os/Zephyr/DefaultQueue.cpp
// \brief sets default Os::Queue to Zephyr implementation via linker
// ======================================================================
#include "Os/Queue.hpp"
#include "Zephyr/Os/Queue.hpp"
#include "Os/Delegate.hpp"

namespace Os {
QueueInterface* QueueInterface::getDelegate(QueueHandleStorage& aligned_new_memory) {
    return Os::Delegate::makeDelegate<QueueInterface, Os::Zephyr::ZephyrQueue>(aligned_new_memory);
}
}
