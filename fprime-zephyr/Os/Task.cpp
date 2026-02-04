// ======================================================================
// \title Os Zephyr/Task.cpp
// \brief implementation of Zephyr implementation of Os::Task
// Relevant Documentation:
// https://docs.zephyrproject.org/latest/doxygen/html/group__thread__apis.html
// https://docs.zephyrproject.org/latest/kernel/services/threads/index.html
// ======================================================================
#include <cstring>

#include <zephyr/kernel.h>
#include "Fw/Logger/Logger.hpp"
#include "Fw/Types/Assert.hpp"
#include "Os/Task.hpp"
#include "fprime-zephyr/Os/Task.hpp"



namespace Os {
namespace Zephyr {
namespace Task {
    void zephyrEntryWrapper(
            void *p1, //!< Pointer to `Task::TaskRoutineWrapper`
            void *p2, //!< Unused
            void *p3 //!< Unused
        ) {
        Os::Task::TaskRoutineWrapper *task = reinterpret_cast<Os::Task::TaskRoutineWrapper *>(p1);
        // Call the task's entry point
        task->invoke();
    }
    void ZephyrTask::onStart() {}

    Os::Task::Status ZephyrTask::start(const Arguments& arguments) {
        size_t stack_size = static_cast<size_t>(arguments.m_stackSize);

        k_thread_stack_t *stack = k_thread_stack_alloc(stack_size, 0);
        FwSizeType priority = arguments.m_priority;

        if (stack == nullptr) {
            return Os::Task::Status::ERROR_RESOURCES;
        }

        // Zephyr priroties range from -16 to +14
        if (priority > 14) {
            Fw::Logger::log("Priority of %d exceeds maximum value. Setting to a priority value of 14.\n", priority);
            priority = 14;
        }

        k_tid_t tid = k_thread_create(&this->m_handle.m_task_descriptor, stack, arguments.m_stackSize, zephyrEntryWrapper, arguments.m_routine_argument, nullptr, nullptr, priority, 0, K_NO_WAIT);


#ifdef CONFIG_THREAD_NAME
        k_thread_name_set(&this->m_handle.m_task_descriptor, arguments.m_name.toChar());
#endif
        k_thread_start(tid);
        return Os::Task::Status::OP_OK;
    }

    Os::Task::Status ZephyrTask::join() {
        int status = k_thread_join(&this->m_handle.m_task_descriptor, K_MSEC(0));
        if(status == 0){
            return Os::Task::Status::OP_OK;
        }
        return Os::Task::Status::JOIN_ERROR;
    }

    TaskHandle* ZephyrTask::getHandle() {
        return &this->m_handle;
    }

    // Note: not implemented for Zephyr threads. Must be manually done using a mutex or other blocking construct as there
    // is no top-level pthreads support for suspend and resume.
    void ZephyrTask::suspend(Os::Task::SuspensionType suspensionType) {
        FW_ASSERT(0);
    }

    void ZephyrTask::resume() {
        FW_ASSERT(0);
    }


    Os::Task::Status ZephyrTask::_delay(Fw::TimeInterval interval) {
        uint32_t delay_ms = interval.getSeconds() * 1000 + interval.getUSeconds() / 1000;
        k_sleep(K_MSEC(delay_ms)); // Need better error checking
        return Os::Task::Status::OP_OK;
    }


} // end namespace Task
} // end namespace Zephyr
} // end namespace Os
