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
#include <libtfc/table.h>

using namespace Tfc;

void BlobTable::add(BlobRecord* row) {
    this->map.insert({ row->getNonce(), row });
    this->_size++;
}

BlobRecord* BlobTable::get(uint32_t nonce) {
    auto row = this->map.find(nonce);
    if(row == this->map.end())
        return nullptr;
    return row->second;
}

void BlobTable::remove(BlobRecord *record) {
    this->map.erase(record->getNonce());
    this->_size--;
}

uint32_t BlobTable::size() {
    return this->_size;
}

std::map<uint32_t, BlobRecord *>::iterator BlobTable::begin() {
    return this->map.begin();
}

std::map<uint32_t, BlobRecord *>::iterator BlobTable::end() {
    return this->map.end();
}

/**
 * Adds a new tag to the table.
 *
 * @param row A pointer to the row to add.
 */
void TagTable::add(TagRecord* row) {
    this->nonceMap.insert({ row->getNonce(), row });
    this->nameMap.insert({ row->getName(), row });
    this->_size++;
}

/**
 * Retrieves a tag row given a nonce.
 *
 * @param nonce A unique ID for the tag
 * @return The tag's row if it exists, otherwise nullptr.
 */
TagRecord* TagTable::get(uint32_t nonce) {
    auto row = this->nonceMap.find(nonce);
    if(row == this->nonceMap.end())
        return nullptr;
    return row->second;
}

/**
 * Retrieves a tag row given a unique name.
 *
 * @param nonce A unique name for the tag.
 * @return The tag's row if it exists, otherwise nullptr.
 */
TagRecord* TagTable::get(std::string name) {
    auto row = this->nameMap.find(name);
    if(row == this->nameMap.end())
        return nullptr;
    return row->second;
}

/**
 * Removes a record from the table.
 *
 * @param record The record to be removed.
 */
void TagTable::remove(TagRecord *record) {
    this->nameMap.erase(record->name);
    this->nonceMap.erase(record->getNonce());
    this->_size--;
};

/**
 * Returns the number of tags in the table.
 */
uint32_t TagTable::size() {
    return this->_size;
}

std::map<std::string, TagRecord*>::iterator TagTable::begin() {
    return this->nameMap.begin();
}

std::map<std::string, TagRecord*>::iterator TagTable::end() {
    return this->nameMap.end();
}
