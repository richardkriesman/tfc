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

#include <tfc/engine/engine.h>
#include <tfc/engine/scribe.h>
#include <tfc/exception.h>

Tfc::Engine::Engine(const std::string &filename) : scribe(filename) {
    this->filename = filename;
    this->isEncrypted = false;
    this->isUnlocked = false;
}

/**
 * Writes a 512 bytes of arbitrary data to a block at the specified position.
 *
 * Each block has a tail section containing a 64-bit file position of the next block in a chain. This
 * can be used for storing files larger than 512 bytes across multiple blocks. If no block is after this
 * block in the chain, this value should be 0.
 *
 * When writing to a block, the block must already be allocated in the container. If a block at the
 * specified index has not been allocated, it will overwrite whatever data is at that position.
 *
 * @param index The index of the block to which data should be written.
 * @param data The 512 bytes of data to write. There must be exactly 512 bytes.
 * @param nextBlock The position of the next block in the chain of blocks composing a file. If
 */
void Tfc::Engine::writeToBlock(uint32_t index, const char *data, uint64_t nextBlock) {
    if (this->scribe.getMode() != OperationMode::EDIT) {
        this->scribe.setMode(OperationMode::EDIT);
    }

    this->moveToBlock(index);
    this->scribe.writeBytes(data, BLOCK_DATA_SIZE); // data section
    this->scribe.writeUInt64(nextBlock);            // tail section
}

/**
 * Moves the cursor to the beginning of a block with the specified index.
 *
 * @param index The index of the block to move to.
 */
void Tfc::Engine::moveToBlock(uint32_t index) {
    this->scribe.setCursorPos(this->blockPos + index * (BLOCK_DATA_SIZE + BLOCK_TAIL_SIZE));
}

/**
 * Moves the cursor to the beginning of the header.
 */
void Tfc::Engine::moveToHeader() {
    this->scribe.setCursorPos(0);
}

/**
 * Sets the Engine's operating mode. The Engine must be the correct mode for an operation to perform
 * that operation.
 *
 * @param mode The mode to set.
 * @throw Exception Failed to open the file with the specified mode.
 */
void Tfc::Engine::setMode(OperationMode mode) {

    // set the scribe's operation mode
    this->scribe.setMode(mode);

    // change the engine's state to work with the new operation mode
    char dek[HEADER_DEK_SIZE];
    uint32_t magicNumber;
    uint32_t version;
    if (mode == OperationMode::READ) { // read file metadata and build in-memory graph

        /*
         * Verify header
         */
        this->moveToHeader();

        // verify that magic number matches
        magicNumber = this->scribe.readUInt32();
        if (magicNumber != MAGIC_NUMBER) {
            throw Exception("Not a valid container file. Magic number does not match.");
        }

        // check file version
        version = this->scribe.readUInt32();
        if (version > CONTAINER_VERSION) {
            throw Exception("Container version mismatch. Must be <= " + std::to_string(CONTAINER_VERSION));
        }

        // check if the file is encrypted (if not, DEK will be all 0s)
        this->scribe.readBytes(dek, HEADER_DEK_SIZE);
        for (char byte : dek) {
            if (byte != 0x0) { // byte is not 0, stream is encrypted
                this->isEncrypted = true;
                this->isUnlocked = false;
            }
        }

        /*
         * Read block list
         */

        // read in the number of blocks
        this->blockCount = this->scribe.readUInt32();

        // save starting position of blocks
        this->blockPos = this->scribe.getCursorPos();

        // move past the block section
        this->moveToBlock(this->blockCount);

        /*
         * Read tag table
         */
        this->tagTablePos = this->scribe.getCursorPos();

        // read next tag nonce and tag count
        this->tagNonce = this->scribe.readUInt32();
        this->tagCount = this->scribe.readUInt32();

        // alloc new tag table (and dealloc any old one)
        delete this->tagTable;
        this->tagTable = new TagTable();

        // build in-memory tag table
        for (uint32_t i = 0; i < this->tagCount; i++) {
            uint32_t nonce = this->scribe.readUInt32();   // nonce that uniquely identifies this tag
            std::string name = this->scribe.readString(); // tag name

            this->tagTable->add(new TagRecord(nonce, name)); // add tag to tag table
        }

        /*
         * Read file table
         */
        this->fileTablePos = this->scribe.getCursorPos();

        // read next file nonce and file count
        this->fileNonce = this->scribe.readUInt32();
        this->fileCount = this->scribe.readUInt32();

        // read file table entries
        for (uint32_t i = 0; i < this->tagCount; i++) {
            uint32_t nonce = this->scribe.readUInt32();
            std::string name = this->scribe.readString();
            uint64_t hash = this->scribe.readUInt64();
            uint64_t startPos = this->scribe.readUInt64();
            uint64_t size = this->scribe.readUInt64();

            // build blob record
            BlobRecord* file = new BlobRecord(nonce, name, hash, startPos, size);

            // read in tags
            uint32_t tagCount = this->scribe.readUInt32();
            for (uint32_t j = 0; j < tagCount; j++) {
                uint32_t tagNonce = this->scribe.readUInt32();

                // get tag from the tag table
                TagRecord* tag = this->tagTable->get(tagNonce);
                if (tag == nullptr) { // tag doesn't, skip this one
                    continue;
                }

                // link tag and file together
                file->addTag(tag);
                tag->addBlob(file);

            }

            // add file to file table
            this->fileTable->add(file);

        }

    }

}
