
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

JumpTableList *JumpTable::list() {
    auto* list = new JumpTableList();
    list->count = this->size();
    list->rows = new JumpTableRow*[list->count];

    uint32_t i = 0;
    for(std::pair<uint32_t, JumpTableRow*> row : this->map) {
        list->rows[i] = row.second;
        i++;
    }

    return list;
}

JumpTableRow::JumpTableRow(uint32_t nonce, const char* hash, std::streampos start) {
    this->nonce = nonce;
    for(int i = 0; i < 32; i++)
        this->hash[i] = hash[i];
    this->start = start;
}
