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
