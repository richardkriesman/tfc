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
#include "OldTask.h"

OldTask::OldTask(const std::function<void*()> &handler) {
    std::lock_guard<std::mutex> lock(this->lock); // acquire state lock

    // initialize state
    this->state = OldTaskState::OLD_PENDING;
    this->result = nullptr;

    // set the handler
    this->handler = handler;
}

OldTask::~OldTask() {
    delete this->thread;
}

/**
 * Blocks the calling thread until the Task has either completed or failed. If it completes, the result of the Task will
 * be returned. If it failed, an exception will be thrown.
 *
 * @return The result of the Task.
 */
void* OldTask::await() {

    // acquire the state lock
    std::unique_lock<std::mutex> lock(this->lock);

    // wait for a notification
    this->event.wait(lock);

    // check the task state
    OldTaskState state = this->state;
    void* result = this->result;
    lock.unlock();
    if(state == OldTaskState::OLD_FAILED)
        throw this->getException();

    return result;
}

/**
 * Returns the exception raised that caused the Task to fail.
 */
std::exception OldTask::getException() {
    auto* exPtr = static_cast<std::exception*>(this->result);
    std::exception ex = *exPtr;
    delete exPtr;
    return ex;
}

/**
 * Returns the result produced by the Task.
 */
void*& OldTask::getResult() { // turns out you need to return it by ref, otherwise this returns nullptr
    return this->result;
}

/**
 * Returns the current state of the Task.
 */
OldTaskState OldTask::getState() {
    std::lock_guard<std::mutex> lock(this->lock);
    return this->state;
}

/**
 * Schedules the task to be performed in a background thread.
 *
 * Tasks are one-off operations. If this function is called after the Task has already been scheduled, an exception
 * will be thrown.
 */
void OldTask::schedule() {
    std::lock_guard<std::mutex> lock(this->lock); // acquire state lock
    if(this->state != OldTaskState::OLD_PENDING)
        throw "Task has already been scheduled";

    // update the task state
    this->state = OldTaskState::OLD_SCHEDULED;

    // create a new thread
    this->thread = new std::thread(runner, this);
    this->thread->detach();
}

/**
 * Runs on the worker thread, setting state and preparing to hand off control to the handler, then updates state and
 * notifies awaiting threads after the handler ends.
 */
void OldTask::runner(OldTask* task) {

    // acquire the state lock
    std::unique_lock<std::mutex> lock(task->lock);

    // set the task as in progress
    task->state = OldTaskState::OLD_RUNNING;
    lock.unlock();

    // hand off task to the handler
    try {
        void* result = task->handler();

        // handler ended successfully, update state and result
        lock.lock();
        task->state = OldTaskState::OLD_COMPLETED;
        task->result = result;
    } catch (std::exception &ex) { // exception was thrown, update state
        lock.lock();
        task->state = OldTaskState::OLD_FAILED;

        // set exception as result
        auto* exPtr = new std::exception();
        *exPtr = ex;
        task->result = static_cast<void*>(exPtr);
    }

    // notify waiting threads of the new result
    task->event.notify_all();
    lock.unlock();

}
