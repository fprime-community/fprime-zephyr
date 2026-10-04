// Mock Os::Task: records the routine so the test can run it inline
#ifndef OS_TASK_HPP
#define OS_TASK_HPP
#include <Fw/FPrimeBasicTypes.hpp>
#include <cstring>
namespace Os {
class TaskString {
  public:
    explicit TaskString(const char* s) : m_s(s) {}
    const char* toChar() const { return m_s; }
  private:
    const char* m_s;
};
class Task {
  public:
    enum Status { OP_OK, INVALID_HANDLE, INVALID_PARAMS, INVALID_STACK, UNKNOWN_ERROR, INVALID_STATE, ERROR_RESOURCES, ERROR_PERMISSION, DELAY_ERROR, JOIN_ERROR, NOT_SUPPORTED };
    static constexpr FwSizeType TASK_DEFAULT = 0;
    typedef void (*taskRoutine)(void*);
    struct Arguments {
        Arguments(const TaskString& name, taskRoutine routine, void* arg, FwTaskPriorityType priority = 0, FwSizeType stackSize = TASK_DEFAULT, FwSizeType cpuAffinity = TASK_DEFAULT)
            : m_name(name), m_routine(routine), m_routine_argument(arg), m_priority(priority), m_stackSize(stackSize), m_cpuAffinity(cpuAffinity) {}
        TaskString m_name;
        taskRoutine m_routine;
        void* m_routine_argument;
        FwTaskPriorityType m_priority;
        FwSizeType m_stackSize;
        FwSizeType m_cpuAffinity;
    };
    // Test control: next start() result and the last recorded arguments
    static Status s_nextStartStatus;
    static taskRoutine s_routine;
    static void* s_routineArg;
    static FwTaskPriorityType s_priority;
    static FwSizeType s_stackSize;
    static int s_startCount;
    static int s_joinCount;

    Status start(const Arguments& args) {
        s_startCount++;
        s_routine = args.m_routine;
        s_routineArg = args.m_routine_argument;
        s_priority = args.m_priority;
        s_stackSize = args.m_stackSize;
        return s_nextStartStatus;
    }
    Status join() {
        s_joinCount++;
        return OP_OK;
    }
};
}  // namespace Os
#endif
