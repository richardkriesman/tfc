
#include "BlobTable.h"

using namespace Tfc;

void BlobTable::add(BlobRecord* row) {
    this->map.insert({ row->nonce, row });
    this->_size++;
}

BlobRecord* BlobTable::get(uint32_t nonce) {
    auto row = this->map.find(nonce);
    if(row == this->map.end())
        return nullptr;
    return row->second;
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

