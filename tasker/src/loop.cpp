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

#include <iostream>
#include <tasker/tasker.h>

using namespace Tasker;

/**
 * Destroys the Loop along with all handles. Tasks will *not* be destroyed, and its state will remain what it was
 * when its handle was destroyed.
 */
Loop::~Loop() {
    std::lock_guard<std::mutex> lock(this->mutex); // acquire the lock

    // clear the task queue
    while(!this->taskQueue.empty()) {
        TaskHandle* handle = this->taskQueue.front();
        this->taskQueue.pop();
        delete handle;
    }
}

/**
 * Starts an event processing loop in this thread.
 */
void Loop::loop(Loop* loop) {
    bool shouldStop = false;
    while(!shouldStop) { // continue loop until it's told to stop
        std::unique_lock<std::mutex> lock(loop->mutex);

        // if no tasks in queue, wait for a task to be added
        if (loop->taskQueue.empty()) {
            lock.unlock();
            loop->scheduled.waitFor(std::chrono::seconds(3));
            lock.lock();
        }

        // run a task in the queue
        if (!loop->taskQueue.empty()) {

            // get task from queue
            TaskHandle* handle = loop->taskQueue.front();
            lock.unlock();

            // start running the task in another thread or resume the task if resumed
            if (handle->task->getState() == SUSPENDED) { // thread was resumed, resume it
                handle->resume();
            } else { // new task, create a thread
                std::thread thread(&handle->exec, handle);
                thread.detach();
            }

            // wait for a context change from the handler
            handle->waitForContextChange();

            // determine what the context change was
            lock.lock();
            switch (handle->task->getState()) {

                // task was resumed by yield(), put task at end of queue and go to next cycle
                case SUSPENDED:
                    loop->taskQueue.pop();
                    loop->taskQueue.push(handle);
                    break;

                // task is done (completed or failed)
                case COMPLETED:
                case FAILED:
                    loop->taskQueue.pop();
                    break;

                // something else we don't care about
                default:
                    break;

            }
            lock.unlock();

        } else {
            lock.unlock();
        }

        // check if the loop should stop
        lock.lock();
        shouldStop = loop->shouldStop && loop->taskQueue.empty(); // task queue must be empty to stop
        lock.unlock();

    }

    // notify waiting threads that the Looper has stopped
    loop->stopped.raise();
}

/**
 * Starts an event processing loop in a new detached thread. This function is non-blocking.
 */
void Loop::start() {
    std::thread thread(&this->loop, this);
    thread.detach();
}

/*
 * Starts an event processing loop in the current thread. This function blocks until the loop is stopped.
 */
void Loop::startInForeground() {
    this->loop(this);
}

/**
 * Signals the Loop to stop peacefully. The Looper will finish the current queue and then stop.
 *
 * This operation is asynchronous. To block until the loop exist, use Loop::wait().
 */
void Loop::stop() {
    std::lock_guard<std::mutex> lock(this->mutex);
    this->shouldStop = true;
    this->scheduled.raise();
}

/**
 * Schedules a Task to be run by the Loop.
 *
 * @param task The Task to be run.
 */
void Loop::run(Task *task) {
    std::lock_guard<std::mutex> lock(this->mutex);

    // set task status to scheduled
    task->setState(TaskState::SCHEDULED);

    // build a ControlHandle for the Task
    auto* handle = new TaskHandle(task);
    this->taskQueue.push(handle);

    // notify the loop that a new task was scheduled
    this->scheduled.raise();
}

/**
 * Blocks the current thread until the Loop has stopped.
 */
void Loop::wait() {
    this->stopped.wait();
}
