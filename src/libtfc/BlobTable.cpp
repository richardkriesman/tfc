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
#include "libtfc/BlobTable.h"

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
