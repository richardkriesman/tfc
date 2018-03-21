/*
 * Tagged File Containers
 * Copyright (C) 2018 Richard Kriesman.
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef TFC_TASKER_H
#define TFC_TASKER_H

#include <condition_variable>
#include <exception>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>

namespace Tasker {

    class Task;
    class TaskHandle;
    class Event;

    static std::mutex stdoutMutex; // a mutex for synchronizing access to stdout

    enum TaskState {
        PENDING,    // task has not been attached to a tasker yet
        SCHEDULED,  // task has been scheduled for execution, but not yet started
        RUNNING,    // task is running on the tasker
        SUSPENDED,  // task has been resumed because it yielded or is awaiting another task
        COMPLETED,  // task completed successfully
        FAILED      // task threw an exception
    };


    class Looper {

    public:
        ~Looper();
        void start();
        void startInForeground();
        void stop();
        void run(Task* task);
        void wait();

    private:

        // task vars
        std::queue<TaskHandle*> taskQueue{};

        // synchronization vars
        bool shouldStop = false;
        std::mutex mutex;
        std::condition_variable taskScheduledCond;
        std::condition_variable stopCond;

        // functions
        static void loop(Looper* looper);

    };

    class Event {

    public:
        void raise();
        void wait();

    private:
        std::mutex mutex;
        std::condition_variable cond;

    };


    class Task {

    public:
        explicit Task(const std::function<void*(TaskHandle* handle)> &runner); // NOLINT
        std::exception_ptr getException();
        void* getResult();
        TaskState getState();
        void* wait();

    private:

        // state vars
        enum TaskState state;   // current lifecycle state of the task
        std::mutex mutex;       // mutex for synchronizing task metadata access
        void* result = nullptr; // result value
        std::exception_ptr ex;  // exception storage
        Event done;             // event raised when the task is done (complete or failed)

        // execution vars
        std::function<void*(TaskHandle* handle)> runner; // the function to be executed asynchronously

        // functions
        static void run(TaskHandle* handle);
        void setState(TaskState state);

        friend class TaskHandle;
        friend class Looper;

    };



    class TaskException : std::exception {

    public:
        explicit TaskException(const std::string &message);
        const char* what();

    private:
        std::string message;

    };


    class TaskHandle {

    public:
        void yield();
        void printf(std::string fmt, ...);

    private:
        explicit TaskHandle(Task* task);
        ~TaskHandle();

        // execution context vars
        Task* task;                          // task being run by this ControlHandle
        std::thread* thread = nullptr;       // thread running the task

        // synchronization
        std::mutex mutex;     // mutex for controlling context access
        Event resumed;        // task is yielding execution
        Event contextChanged; // the execution context changed

        // tasker internal functions
        static void exec(TaskHandle* handle);
        void kill();
        void resume();
        void waitForContextChange();

        friend class Task;
        friend class Looper;

    };

}

#endif //TFC_TASKER_H
