
#include "TagTable.h"

using namespace Tfc;

/**
 * Adds a new tag to the table.
 *
 * @param row A pointer to the row to add.
 */
void TagTable::add(TagRecord* row) {
    this->nonceMap.insert({ row->nonce, row });
    this->nameMap.insert({ row->name, row });
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
 * Returns the number of tags in the table.
 */
uint32_t TagTable::size() {
    return this->_size;
}

std::map<uint32_t, TagRecord*>::iterator TagTable::begin() {
    return this->nonceMap.begin();
}

std::map<uint32_t, TagRecord*>::iterator TagTable::end() {
    return this->nonceMap.end();
};

