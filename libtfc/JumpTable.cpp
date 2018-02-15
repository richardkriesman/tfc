
#include "JumpTable.h"

using namespace Tfc;

void JumpTable::add(JumpTableRow* row) {
    this->map.insert({ row->nonce, row });
    this->_size++;
}

JumpTableRow* JumpTable::get(uint32_t nonce) {
    auto row = this->map.find(nonce);
    if(row == this->map.end())
        return nullptr;
    return row->second;
}

uint32_t JumpTable::size() {
    return this->_size;
}

JumpTableRow::JumpTableRow(uint32_t nonce, char* hash, std::streampos start) {
    this->nonce = nonce;
    strcpy(this->hash, hash);
    this->start = start;
}
