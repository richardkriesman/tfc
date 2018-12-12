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
#include <tfc/record.h>
#include <tfc/container.h>

using namespace Tfc;

Record::Record(uint32_t nonce) {
    this->nonce = nonce;
}

/**
 * Binary function for use in std::sort to sort in ascending order.
 *
 * @return Whether record1 should be before record2.
 */
bool Record::asc(Record* record1, Record* record2) {
    return record1->nonce < record2->nonce;
}

/**
 * Binary function for use in std::sort to sort in descending order.
 *
 * @return Whether record1 should be after record2.
 */
bool Record::desc(Record* record1, Record* record2) {
    return record1->nonce > record2->nonce;
}

FileRecord::FileRecord(uint32_t nonce, const std::string &name, uint64_t hash, uint32_t start, uint64_t size) : Record(nonce) {
    this->nonce = nonce;
    this->name = name;
    this->hash = hash;
    this->start = start;
    this->size = size;
}

void FileRecord::addTag(Tfc::TagRecord* tag) {
    this->tags.push_back(tag);
}

TagRecord::TagRecord(uint32_t nonce, const std::string &name) : Record(nonce) {
    this->nonce = nonce;
    this->name = name;
}

void TagRecord::addBlob(Tfc::FileRecord *blob) {
    this->files.push_back(blob);
}
