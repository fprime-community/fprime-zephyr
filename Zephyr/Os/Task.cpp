#include <Os/Task.hpp>
#include <Fw/Logger/Logger.hpp>
#include <Fw/Types/Assert.hpp>

#include <zephyr/kernel.h>

namespace Os {

    namespace {

        //! Zephyr thread entry function expects three arguments. This is a glue that creates such function.
        void zephyrEntryWrapper(
              void *p1, //!< Pointer to `Task::TaskRoutineWrapper`
              void *p2, //!< Unused
              void *p3 //!< Unused
          ) {
            Task::TaskRoutineWrapper *task = reinterpret_cast<Task::TaskRoutineWrapper *>(p1);
            // Call the task's entry point
            task->routine(task->arg);
          }
    }

    Task::Task() : m_handle(reinterpret_cast<POINTER_CAST>(nullptr)), m_identifier(0), m_affinity(-1), m_started(false), m_suspendedOnPurpose(false), m_routineWrapper() {}

    Task::TaskStatus Task::start(const Fw::StringBase &name, taskRoutine routine, void* arg, NATIVE_UINT_TYPE priority, NATIVE_UINT_TYPE stackSize,  NATIVE_UINT_TYPE cpuAffinity, NATIVE_UINT_TYPE identifier) {
        FW_ASSERT(cpuAffinity == Task::TASK_DEFAULT); // Zephyr SMP support is not implemented.
        FW_ASSERT(routine);

        this->m_name = "T_";
        this->m_name += name;
        this->m_identifier = identifier;
        this->m_started = false;
        this->m_affinity = cpuAffinity;
        this->m_routineWrapper.routine = routine;
        this->m_routineWrapper.arg = arg;

        k_thread_stack_t *stack = k_thread_stack_alloc(stackSize, 0);
        
        k_thread *thread = reinterpret_cast<k_thread*>(k_object_alloc(K_OBJ_THREAD));
        if (thread == nullptr) {
            return TASK_ERROR_RESOURCES;
        }

        // Zephyr priroties range from -16 to +14
        if (priority > 14) {
            Fw::Logger::logMsg("Priority of %d exceeds maximum value. Setting to a priority value of 14.\n", priority);
            priority = 14;
        }

        k_tid_t tid = k_thread_create(thread, stack, stackSize, zephyrEntryWrapper, &this->m_routineWrapper, nullptr, nullptr, priority, 0, K_NO_WAIT);
#ifdef CONFIG_THREAD_NAME
        int ret = k_thread_name_set(thread, this->m_name.toChar());
        FW_ASSERT(ret == 0, ret);
#endif
        k_thread_start(tid);

        this->m_handle = reinterpret_cast<POINTER_CAST>(tid);
        Task::s_numTasks++;
        // If a registry has been registered, register task
        if (Task::s_taskRegistry) {
            Task::s_taskRegistry->addTask(this);
        }

        // Uncomment to allow threads allocating memory from kernel heap.
        // k_thread_system_pool_assign(thread);
        return TASK_OK;
    }


    Task::~Task() {
        if (this->m_handle) {
            k_object_release(reinterpret_cast<k_thread*>(this->m_handle));
    	}
        // If a registry has been registered, remove task
        if (Task::s_taskRegistry) {
            Task::s_taskRegistry->removeTask(this);
        }
    }

    Task::TaskStatus Task::delay(NATIVE_UINT_TYPE milliseconds)
    {
        return Task::TASK_DELAY_ERROR; // TODO
    }

    void Task::suspend(bool onPurpose) {
        FW_ASSERT(0);
    }

    void Task::resume() {
        FW_ASSERT(0);
    }

    bool Task::isSuspended() {
        FW_ASSERT(0);
        return false;
    }

    TaskId Task::getOsIdentifier() {
        TaskId T;
        return T;
    }

    Task::TaskStatus Task::join(void **value_ptr) {
        NATIVE_INT_TYPE stat = 0;
        if (!(this->m_handle)) {
            return TASK_JOIN_ERROR;
        }
        stat = k_thread_join(reinterpret_cast<k_thread*>(this->m_handle), K_MSEC(0));

        if (stat != 0) {
            return TASK_JOIN_ERROR;
        }
        else {
            return TASK_OK;
        }
    }
}