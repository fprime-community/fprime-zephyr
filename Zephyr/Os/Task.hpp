// ======================================================================
// \title Os/Zephyr/Task.hpp
// \brief definitions of Zephyr implementation of Os::Task
// ======================================================================
#ifndef Os_Zephyr_Task_hpp_
#define Os_Zephyr_Task_hpp_
#include <Fw/FPrimeBasicTypes.hpp>
#include <Fw/Types/Serializable.hpp>
#include <Os/TaskString.hpp>
#include <Os/Mutex.hpp>
#include <Fw/Deprecate.hpp>
#include <zephyr/kernel.h>
#include <Os/Task.hpp>
namespace Os {
namespace Zephyr {
namespace Task {

    //! TaskHandle class definition for Zephyr implementations.
    //!
    struct ZephyrTaskHandle : public TaskHandle {
        k_thread* m_task_descriptor;
    };

    //! Zephyr task implementation as driven by pthreads implementation
    class ZephyrTask : public TaskInterface {
      public:

        //! \brief default constructor
        ZephyrTask() = default;

        //! \brief default virtual destructor
        ~ZephyrTask() = default;

        //! \brief copy constructor is forbidden
        ZephyrTask(const ZephyrTask& other) = delete;

        //! \brief assignment operator is forbidden
        ZephyrTask& operator=(const ZephyrTask& other) = delete;


        //! \brief perform required task start actions
        void onStart() override;

        //! \brief start the task
        //!
        //! Starts the task given the supplied arguments. This is done via a task routine wrapper intermediary that
        //! ensures that `setStarted` is called once the task has actually started to run. The task then runs the user
        //! routine. This function may return before the new task begins to run.
        //
        //! It is illegal for arguments.m_routine to be null.
        //!
        //! \param arguments: arguments supplied to the task start call
        //! \return status of the task start
        Status start(const Arguments& arguments) override;

        //! \brief block until the task has ended
        //!
        //! Blocks the current (calling) task until this task execution has ended. Callers should ensure that any
        //! signals required to stop this task have already been emitted or will be emitted by another task.
        //!
        //! \return status of the block
        Status join() override; //!< Wait for task to finish

        //! \brief suspend the task given the suspension type
        //!
        //! Suspends the task. Some implementations track if the suspension of a task was intentional or
        //! unintentional. The supplied `suspensionType` parameter indicates that this was intentional or
        //! unintentional. The type of suspension is also returned when calling `isSuspended`.
        //!
        //! \param suspensionType intentionality of the suspension
        void suspend(SuspensionType suspensionType) override;

        //! \brief resume a suspended task
        //!
        //! Resumes this task. Not started, running, and exited tasks take no action.
        //!
        void resume() override;

        //! \brief delay the current task
        //!
        //! Delays, or sleeps, the current task by the supplied time interval. In non-preempting os implementations
        //! the task will resume no earlier than expected but an exact wake-up time is not guaranteed.
        //!
        //! \param interval: delay time
        //! \return status of the delay
        Status _delay(Fw::TimeInterval interval) override;

        //! \brief return the underlying task handle (implementation specific)
        //! \return internal task handle representation
        TaskHandle* getHandle() override;
      PRIVATE:
        ZephyrTaskHandle m_handle; //!< Zephyr task tracking
    };
} // end namespace Task
} // end namespace Zephyr
} // end namespace Os
#endif