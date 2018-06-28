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
#include <tasker/tasker.h>

using namespace Tasker;

/**
 * Creates a new TaskException, which contains a string describing the particular problem that occurred.
 *
 * Bear in mind that should an error be thrown in a Task, only an std::exception object will be returned. TaskExceptions
 * set their what() value to the message string provided, so this is a good way to pass a specific error message back
 * to the issuing thread.
 *
 * @param message A message describing the problem that caused the exception.
 */
TaskException::TaskException(const std::string &message) {
    this->message = message;
}

/**
 * Returns a message describing the problem that caused the exception.
 */
const char *TaskException::what() {
    return this->message.c_str();
}
