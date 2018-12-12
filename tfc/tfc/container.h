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
#ifndef TFC_TFC_FILE_H
#define TFC_TFC_FILE_H

#include <string>
#include <fstream>
#include <arpa/inet.h>
#include <chrono>
#include <tfc/exception.h>
#include <tfc/table.h>
#include <tfc/engine/engine.h>
#include "file.h"

namespace Tfc {

    struct Blob {
        FileRecord* record;
        char* data;
    };


    class Container {

    public:
        explicit Container(const std::string &filename);

        ReadableFile* readFile(uint32_t nonce);

        uint32_t                 addBlob(const std::string &name, char* bytes, uint64_t size);
        void                     attachTag(uint32_t nonce, const std::string &tag);
        void                     deleteBlob(uint32_t nonce);
        bool                     doesExist();
        OperationMode            getMode();
        void                     init();
        std::vector<FileRecord*> intersection(const std::vector<std::string> &tags);
        bool                     isEncrypted();
        bool                     isUnlocked();
        std::vector<FileRecord*> listBlobs();
        std::vector<TagRecord*>  listTags();
        void                     mode(OperationMode mode);
        Blob*                    readBlob(uint32_t nonce);

    private:
        Engine engine; // "engine" for reading/writing data from/to the container

        // constants
        const uint32_t CONTAINER_CONSTANT = 1;
        const uint32_t MAGIC_NUMBER       = 0xE621126E;

        // header field lengths (in bytes)
        const unsigned int BLOCK_DATA_SIZE       = 512;
        const unsigned int BLOCK_LIST_COUNT_SIZE = 4;
        const unsigned int BLOCK_NEXT_SIZE       = 4;
        const unsigned int BLOCK_SIZE            = 516;
        const unsigned int DEK_LEN               = 32;
        const unsigned int FILE_VERSION_LEN      = 4;
        const unsigned int HASH_BUFFER_SIZE      = 64;
        const unsigned int HASH_LEN              = 32;
        const unsigned int MAGIC_NUMBER_LEN      = 4;
        const unsigned int NONCE_LEN             = 4;

        // file vars
        OperationMode op;         // current operation mode
        std::string filename;     // name of the file
        std::fstream stream;      // file stream
        bool encrypted = false;   // whether the file is encrypted
        bool unlocked = true;     // whether the file is unlocked (true if unencrypted)
        bool exists = false;      // whether the file exists in the filesystem
        uint32_t blockCount = 0;  // number of blocks in the block list

        // file section byte positions
        std::streampos headerPos;     // start position of header
        std::streampos tagTablePos;   // start position of tag table
        std::streampos blobTablePos;  // start position of blob table
        std::streampos blockListPos;   // start position of blob list

        // next auto-increment table nonces
        uint32_t tagTableNextNonce;   // next nonce for a new tag
        uint32_t blobTableNextNonce;  // next nonce for a new blob

        // in-memory tables
        TagTable* tagTable = nullptr;
        BlobTable* blobTable = nullptr;

        void        analyze();
        uint64_t    hash(char* bytes, size_t size);
        void        jump(std::streampos length);
        void        jumpBack(std::streampos length);
        void        next(std::streampos length);
        std::string readString();
        uint32_t    readUInt32();
        uint64_t    readUInt64();
        void        reset();
        void        writeBlobTable();
        void        writeString(const std::string &value);
        void        writeTagTable();
        void        writeUInt32(const uint32_t &value);
        void        writeUInt64(const uint64_t &value);
    };

}

#endif //TFC_TFC_FILE_H