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

#include <cstdarg>
#include <tasker/tasker.h>

using namespace Tasker;

/**
 * Creates a new ControlHandle for a Task. ControlHandles provide thread-safe utility functions as well as functions
 * for managing a Task's own execution context. For Loopers, they provide supervisory control over the Task's execution.
 *
 * @param task The task fo create a ControlHandle for.
 */
TaskHandle::TaskHandle(Task *task) {
    this->task = task;
}

/**
 * Destroys the ControlHandle and forcibly terminates the running thread.
 */
TaskHandle::~TaskHandle() {
    this->kill();
}

/**
 * Starts running the bound Task on the current thread. This function will block until the thread exits, which will
 * cause it to notify listeners of a context change.
 */
void TaskHandle::exec(TaskHandle* handle) {
    std::unique_lock<std::mutex> lock(handle->mutex);

    // run the task
    handle->thread = new std::thread(handle->task->run, handle);
    lock.unlock();
    handle->thread->join();

    // notify the tasker of the context change
    handle->contextChanged.raise();
}

/**
 * Forcibly terminates the bound Task by destroying its thread.
 */
void TaskHandle::kill() {
    std::lock_guard<std::mutex> lock(this->mutex);
    delete this->thread;
}

/**
 * Resumes execution of the Task if it was suspended.
 */
void TaskHandle::resume() {
    this->resumed.raise();
}

/**
 * Blocks execution of a thread until the control handle undergoes a context change. Intended for use by the Loop when
 * executing a Task.
 */
void TaskHandle::waitForContextChange() {
    this->contextChanged.wait();
}

/**
 * Yields execution to the next Task in the scheduler. This Task will be resumed after this call when the
 * scheduler arrives at its next cycle.
 *
 * Long-running or intensive tasks should regularly yield to prevent resource starvation in other Tasks.
 */
void TaskHandle::yield() {

    // update state to suspended
    this->task->setState(TaskState::SUSPENDED);

    // notify the tasker of the context change
    this->contextChanged.raise();

    // wait for suspension to be ended
    this->resumed.wait();

    // update the task's state
    this->task->setState(TaskState::RUNNING);

}

/**
 * A wrapper function for printf with support thread-safety and automatic buffer flushing.
 *
 * @param fmt A printf format string
 * @param ... A series of arguments to interpolate into the format string.
 */
void TaskHandle::printf(std::string fmt, ...) {
    std::lock_guard<std::mutex> lock(stdoutMutex);
    va_list args;
    va_start(args, fmt);
    vprintf(fmt.c_str(), args);
    fflush(stdout);
    va_end(args);
}
