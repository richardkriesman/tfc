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
 * Raises an event, waking up all waiting threads.
 */
void Event::raise() {
    this->cond.notify_all();
}

/**
 * Blocks the current thread until the event is raised.
 */
void Event::wait() {
    std::unique_lock<std::mutex> lock(this->mutex);
    this->cond.wait(lock);
    lock.unlock();
}

/**
 * Blocks the current thread until the event is raised, or until a period of time has passed.
 *
 * @param length The maximum amount of time to wait.
 * @return True if the wait stopped because the event was raised. False if the event timed out.
 */
bool Event::waitFor(std::chrono::seconds const &length) {
    std::unique_lock<std::mutex> lock(this->mutex);
    std::cv_status status = this->cond.wait_for(lock, length);
    lock.unlock();
    return status == std::cv_status::timeout;
}
