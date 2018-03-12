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
