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

#include <tfc/exception.h>
#include <tfc/engine/scribe.h>
#include <tfc/portable_endian.h>

using namespace Tfc;

Tfc::Scribe::Scribe(const std::string &filename) {
    this->filename = filename;
    this->mode = OperationMode::CLOSED;
}

/**
 * WARNING: getCursorPos() may not work correctly if the system definition of std::streampos is > 64 bits!
 *
 * @return The position of the cursor as a 64-bit unsigned integer.
 */
uint64_t Tfc::Scribe::getCursorPos() {
    return static_cast<uint64_t>(this->stream.tellg());
}

/**
 * @return The filename of the file.
 */
std::string Scribe::getFilename() {
    return this->filename;
}

/**
 * @return The Scribe's current operation mode.
 */
OperationMode Tfc::Scribe::getMode() {
    return this->mode;
}

/**
 * Reads bytes directly from the file at a the current position into a buffer, moving the cursor forward
 * by the number of bytes read.
 *
 * @param buf The buffer to read bytes into.
 * @param len The number of bytes to read.
 */
void Tfc::Scribe::readBytes(char* buf, std::streamsize len) {
    this->stream.read(buf, len);
    if (this->stream.fail() || this->stream.bad()) {
        throw Exception("Failed to read bytes at position " + std::to_string(this->getCursorPos()));
    }
}

/**
 * Reads a variable-length string from the file at the current position, moving the cursor forward
 * by the number of bytes in the string.
 *
 * @return The string that was read from the stream.
 * @throws Exception Reading the string from the file failed.
 */
std::string Tfc::Scribe::readString() {
    std::string str; // the string as it's being built

    // read the string into a buffer until the null terminator is reached
    char current;
    do {

        // read char from stream
        this->stream.read(&current, 1);
        if (this->stream.fail()) {
            throw Exception("Failed to read string at position " + std::to_string(this->getCursorPos()));
        }

        // convert char to string and append
        if (current != '\0') { // don't append the null terminator, std::string already handles it
            str += std::string(1, current);
        }

    } while (current != '\0'); // stop reading when we reach the null terminator

    return str;
}

/**
 * Reads a 32-bit unsigned integer from the file at the current position, moving the cursor forward
 * by 4 bytes.
 */
uint32_t Tfc::Scribe::readUInt32() {
    uint32_t value;
    this->stream.read(reinterpret_cast<char*>(&value), sizeof(uint32_t));
    if (this->stream.fail() || this->stream.bad()) {
        throw Exception("Failed to read uint32 at position " + std::to_string(this->getCursorPos()));
    }
    return ntohl(value);
}

/**
 * Reads a 64-bit unsigned integer from the file at the current position, moving the cursor forward
 * by 8 bytes.
 */
uint64_t Tfc::Scribe::readUInt64() {
    uint64_t value;
    this->stream.read(reinterpret_cast<char*>(&value), sizeof(uint64_t));
    if(this->stream.fail()) {
        throw Exception("Failed to read uint64 at position " + std::to_string(this->getCursorPos()));
    }
    return be64toh(value);
}

/**
 * Closes the file stream, resets all flags, and changes the operation mode to CLOSED.
 */
void Tfc::Scribe::reset() {
    this->stream.close();
    this->stream.clear();
    this->mode = OperationMode::CLOSED;
}

/**
 * Moves the cursor to the specified position.
 *
 * @param pos The absolute position to move the cursor to.
 */
void Tfc::Scribe::setCursorPos(uint64_t pos) {
    this->stream.seekg(pos);
}

/**
 * Sets the Scribe's operating mode.
 *
 * @param mode The mode to set.
 * @throw Exception Failed to open the file with the specified mode.
 */
void Tfc::Scribe::setMode(OperationMode mode) {

    // reset the stream if it isn't already closed
    if (this->mode != OperationMode::CLOSED) {
        this->reset();
    }

    // switch file modes
    switch (mode) {

        // file stream is already closed, no need to do anything
        case OperationMode::CLOSED:
            break;

        // open the file for reading
        case OperationMode::READ:
            this->stream.open(this->filename, std::ios::in | std::ios::binary);
            if (this->stream.fail() || this->stream.bad()) {
                throw Exception("Failed to open container for reading");
            }
            this->mode = OperationMode::READ;
            break;

        // open a new file, truncating any existing one
        case OperationMode::CREATE:
            this->stream.open(this->filename, std::ios::out | std::ios::binary);
            if (this->stream.fail() || this->stream.bad()) {
                throw Exception("Failed to open new container");
            }
            this->mode = OperationMode::CREATE;
            break;

        // open an existing file for editing
        case OperationMode::EDIT:
            this->stream.open(this->filename, std::ios::in | std::ios::out | std::ios::binary);
            if (this->stream.fail() || this->stream.bad()) {
                throw Exception("Failed to open container for editing");
            }
            this->mode = OperationMode::EDIT;
            break;

    }

}

/**
 * Writes bytes from a buffer directly into the file at the current position, moving the cursor forward
 * by the number of bytes written.
 *
 * @param buf The buffer whose data should be written.
 * @param len The number of bytes to write.
 */
void Tfc::Scribe::writeBytes(const char *buf, std::streamsize len) {
    this->stream.write(buf, len);
    if (this->stream.fail() || this->stream.bad()) {
        throw Exception("Failed to write bytes at position " + std::to_string(this->getCursorPos()));
    }
}

/**
 * Writes a 64-bit unsigned integer directly into the file at the current position, moving the cursor
 * forward by 8 bytes.
 *
 * @param value The value of the integer to write.
 */
void Tfc::Scribe::writeUInt64(uint64_t value) {
    uint64_t networkByteValue = htobe64(value);
    this->stream.write((char*) &networkByteValue, sizeof(uint64_t));
    if(this->stream.fail()) {
        throw Exception("Failed to write uint64");
    }
}
