
#include "Record.h"

using namespace Tfc;

BlobRecord::BlobRecord(uint32_t nonce, std::string name, const char* hash, std::streampos start) {
    this->nonce = nonce;
    this->name = name;
    for(int i = 0; i < 32; i++)
        this->hash[i] = hash[i];
    this->start = start;
}

void BlobRecord::addTag(Tfc::TagRecord* tag) {
    this->tags.push_back(tag);
}

bool BlobRecord::operator<(BlobRecord *other) {
    return this->nonce < other->nonce;
}

TagRecord::TagRecord(uint32_t nonce, const std::string &name) {
    this->nonce = nonce;
    this->name = name;
}

void TagRecord::addBlob(Tfc::BlobRecord *blob) {
    this->blobs.push_back(blob);
}

bool TagRecord::operator<(TagRecord *other) {
    return this->nonce < other->nonce;
}