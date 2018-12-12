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

#include <tfc/file.h>
#include <tfc/exception.h>

#include "tfc/file.h"

using namespace Tfc;

/**
 * @return Name of the file on disk
 */
std::string File::getFilename() {
    return this->filename;
}

/**
 * @return A file integrity hash
 */
std::uint64_t File::getHash() {
    return this->hash;
}

/**
 * @return Size of the file in bytes
 */
std::uint64_t File::getSize() {
    return this->size;
}

/**
 * @return An immutable array of tags to which this file belongs
 */
std::vector<TagRecord*> File::getTags() {
    return std::vector<TagRecord*>(this->tags);
}

/**
 * @return Unique identifier for this file
 */
uint32_t File::getNonce() {
    return this->nonce;
}

/**
 * Creates a new ReadableFile, which enables reading a file from the container.
 */
ReadableFile::ReadableFile(Engine* engine, FileRecord* metadata) {
    this->engine = engine;
    this->filename = metadata->getName();
    this->hash = metadata->getHash();
    this->size = metadata->getSize();
    this->tags = *metadata->getTags();

    this->remainingBytes = this->size;
    this->index = 0; // TODO: Read *index*, not streampos
}

/**
 * @return Whether the end of the file has been reached.
 */
bool ReadableFile::isEof() {
    return this->remainingBytes > 0;
}

/**
 * Reads exactly 512 bytes from the container, advancing the cursor by 1 block.
 *
 * @return The data content of the block, exactly 512 bytes in size.
 */
char* ReadableFile::readBlock() {
    if (this->isEof()) {
        throw Exception("End of file has been reached");
    }

    block_t* block = this->engine->readBlock(this->index);
    this->index = block->nextBlock;
    this->remainingBytes -= BLOCK_DATA_SIZE;
    return block->data;
}
