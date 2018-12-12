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
#ifndef TFC_EDITOR_H
#define TFC_EDITOR_H

#include <fstream>

namespace Tfc {

    enum OperationMode {
        CLOSED,
        READ,
        CREATE,
        EDIT
    };

    /**
     * The Scribe provides an simplified interface for reading and writing primitive data types to files.
     */
    class Scribe {

    public:
        explicit Scribe(const std::string &filename);

        uint64_t      getCursorPos();
        std::string   getFilename();
        OperationMode getMode();
        void          readBytes(char* buf, std::streamsize len);
        std::string   readString();
        uint32_t      readUInt32();
        uint64_t      readUInt64();
        void          reset();
        void          setCursorPos(uint64_t pos);
        void          setMode(OperationMode mode);
        void          writeBytes(const char* buf, std::streamsize len);
        void          writeUInt64(uint64_t value);

    private:
        std::string   filename; // name of the file on disk
        OperationMode mode;     // the editor's mode of operation
        std::fstream  stream;   // file stream for reading and writing to file

    };

}

#endif //TFC_EDITOR_H
