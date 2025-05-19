// ======================================================================
// \title Os Zephyr/Task.cpp
// \brief implementation of Zephyr implementation of Os::Task
// ======================================================================
#include <cstring>
#include <unistd.h>
#include <climits>
#include <cerrno>
#include <pthread.h>

#include "Fw/Logger/Logger.hpp"
#include "Fw/Types/Assert.hpp"
#include "Os/Task.hpp"


namespace Os {
namespace Zephyr {
namespace Task {
    void zephyrEntryWrapper(
            void *p1, //!< Pointer to `Task::TaskRoutineWrapper`
            void *p2, //!< Unused
            void *p3 //!< Unused
        ) {
        Task::TaskRoutineWrapper *task = reinterpret_cast<Task::TaskRoutineWrapper *>(p1);
        // Call the task's entry point
        task->invoke();
    }
    void ZephyrTask::onStart() {}

    Os::Task::Status ZephyrTask::start(const Arguments& arguments) {
        k_thread_stack_t *stack = k_thread_stack_alloc(arguments.m_stackSize, 0);
        
        k_thread *thread = reinterpret_cast<k_thread*>(k_object_alloc(K_OBJ_THREAD));
        if (thread == nullptr) {
            return Os::Task::Status::ERROR_RESOURCES;
        }

        // Zephyr priroties range from -16 to +14
        if (priority > 14) {
            Fw::Logger::logMsg("Priority of %d exceeds maximum value. Setting to a priority value of 14.\n", priority);
            priority = 14;
        }

        k_tid_t tid = k_thread_create(thread, stack, arguments.m_stackSize, zephyrEntryWrapper, argument.m_routine_argument, nullptr, nullptr, argument.m_priority, 0, K_NO_WAIT);
#ifdef CONFIG_THREAD_NAME
        int ret = k_thread_name_set(thread, this->arguments.m_name.toChar());
#endif
        k_thread_start(tid);
        this->m_handle.m_task_descriptor = thread;
        return Os::Task::Status::OP_OK;
    }

    Os::Task::Status ZephyrTask::join() {
        Os::Task::Status status = Os::Task::Status::JOIN_ERROR;
        status = kthread_join(this->m_handle.m_task_descriptor, K_MSEC(0));
        return status;
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