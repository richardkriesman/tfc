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

#ifndef TFC_ENGINE_H
#define TFC_ENGINE_H

#include <fstream>
#include <mutex>

#include <tfc/table.h>
#include <tfc/engine/scribe.h>

namespace Tfc {

    class Engine {

    public:
        explicit Engine(const std::string &filename);

        void writeToBlock(uint32_t index, const char data[512], uint64_t nextBlock);

    private:
        const uint32_t CONTAINER_VERSION = 1;
        const uint32_t MAGIC_NUMBER = 0xE621126E;

        // structure sizes
        const unsigned int BLOCK_DATA_SIZE = 512;
        const unsigned int BLOCK_TAIL_SIZE = sizeof(std::streampos);
        const unsigned int HEADER_DEK_SIZE = 32;

        // engine state
        std::string    filename;    // name of container on disk
        bool           isEncrypted; // whether the file is encrypted
        bool           isUnlocked;  // whether the file has been unlocked and can be operated on
        std::mutex     opLock;      // mutex lock on operations
        Scribe         scribe;      // scribe for reading and writing to the file

        // structure positions
        uint64_t blockPos;      // starting pos of the blocks (not the block list section)
        uint64_t fileTablePos;  // starting pos of the file table
        uint64_t tagTablePos;   // starting pos of the tag table

        // structure metadata
        uint32_t blockCount; // number of blocks in container
        uint32_t tagCount;   // number of tags in container
        uint32_t tagNonce;   // nonce for the next entry in the file table
        uint32_t fileCount;  // number of files in container
        uint32_t fileNonce;  // nonce for the next entry in the file table

        // graph structures
        TagTable* tagTable = nullptr;
        BlobTable* fileTable = nullptr;

        // functions
        void        moveToBlock(uint32_t index);
        void        moveToHeader();
        void        setMode(OperationMode mode);
    };


}


#endif //TFC_ENGINE_H
