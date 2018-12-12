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

#ifndef TFC_FILE_H
#define TFC_FILE_H

#include <string>

#include <tfc/engine/scribe.h>
#include <tfc/table.h>
#include <tfc/engine/engine.h>

namespace Tfc {

    class File {

    public:
        std::string             getFilename();
        std::uint64_t           getHash();
        std::uint64_t           getSize();
        std::vector<TagRecord*> getTags();

    protected:
        std::string             filename; // the file's name on disk
        uint64_t                hash;     // file hash for verifying integrity
        uint64_t                size;     // the size of the file's contents in bytes
        std::vector<TagRecord*> tags;     // vector of pointers to tags

        uint32_t getNonce();

    private:
        uint32_t nonce; // unique identifier for this record

    };


    class ReadableFile : public Tfc::File {

    public:
        ReadableFile(Engine* engine, FileRecord* metadata);

        bool  isEof();
        char* readBlock();

    private:
        Engine*  engine;         // engine for reading/writing data
        uint64_t remainingBytes; // number of bytes remaining to be read
        uint32_t index;          // index of the current block
    };


    class WritableFile : public Tfc::File {

    };

}

#endif //TFC_FILE_H
