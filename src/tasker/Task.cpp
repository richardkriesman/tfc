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
#include "Tasker.h"

using namespace Tasker;

/**
 * Creates a new Task for performing an asynchronous operation. Tasks are
 * designed to run in the background in a Looper, which implements an event
 * loop.
 *
 * @param runner The function to executed by the Looper. It should accept a
 *               pointer to a ControlHandle, which it can use to control
 *               its execution in the loop and use thread-safe functions.
 */
Task::Task(const std::function<void *(TaskHandle* handle)> &runner) {

    // acquire mutex lock
    std::lock_guard<std::mutex> lock(this->mutex);

    // instantiate vars
    this->state = TaskState::PENDING;
    this->runner = runner;

}

/**
 * Executes the runner on the current thread. This operation is blocking until the runner completes. Task state will be
 * automatically updated.
 *
 * @param handle A pointer to a ControlHandle from the attached Looper. This handle provides the runner with an
 *               interface to access thread-safe utility functions and execution context functions.
 * @return A void pointer to the data returned by the runner.
 */
void Task::run(TaskHandle* handle) {
    Task* task = handle->task;
    task->setState(TaskState::RUNNING);

    // execute the runner
    try {
        task->result = task->runner(handle);
        task->setState(TaskState::COMPLETED);
    } catch (std::exception &ex) { // exception was thrown in runner, task failed
        task->ex = std::current_exception();
        task->setState(TaskState::FAILED);
    }

    // raise the "done" event so we can notify listeners
    handle->task->done.raise();
}

/**
 * Sets the current state of the task. This operation is thread-safe.
 *
 * @param state The new Task state.
 */
void Task::setState(TaskState state) {
    std::lock_guard<std::mutex> lock(this->mutex); // acquire the lock
    this->state = state;
}

/**
 * Returns the current state of the Task.
 */
TaskState Task::getState() {
    std::lock_guard<std::mutex> lock(this->mutex);
    return this->state;
}

/**
 * Returns the result of the Task. If the Task failed, this is an std::exception_ptr.
 */
void *Task::getResult() {
    return this->result;
}

/**
 * Returns an exception_ptr of the exception that required the Task to fail.
 */
std::exception_ptr Task::getException() {
    return this->ex;
}

/**
 * Blocks the current thread until the Task completes. If the Task failed, this will rethrow the exception thrown
 * by the Task.
 *
 * @return The result of the Task.
 */
void *Task::wait() {
    this->done.wait();
    if (this->state == FAILED)
        std::rethrow_exception(this->getException());
    return this->result;
}