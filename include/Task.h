
#ifndef TFC_TASK_H
#define TFC_TASK_H

#include <mutex>
#include <thread>
#include <condition_variable>
#include <functional>

enum TaskState {
    PENDING,
    SCHEDULED,
    RUNNING,
    COMPLETED,
    FAILED
};

class Task {

public:
    explicit Task(const std::function<void*()> &handler);
    ~Task();

    void* await();
    std::exception getException();
    void*& getResult();
    TaskState getState();
    void schedule();


private:

    // state
    enum TaskState state;           // whether the task has been completed successfully
    std::mutex lock;                // mutex for controlling access to task state
    std::condition_variable event;  // condition variable for threads that want to wait for the task to complete
    void* result;                   // result of task

    // thread
    std::thread* thread;            // the worker thread that will process this task
    std::function<void*()> handler; // function pointer to handle the task

    static void runner(Task* task); // runs on the worker thread and prepares to hand it off to the handler

};


#endif //TFC_TASK_H
