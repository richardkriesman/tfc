
#include "Record.h"

using namespace Tfc;

BlobRecord::BlobRecord(uint32_t nonce, const char* hash, std::streampos start) {
    this->nonce = nonce;
    for(int i = 0; i < 32; i++)
        this->hash[i] = hash[i];
    this->start = start;
}

void BlobRecord::addTag(Tfc::TagRecord* tag) {
    this->tags.push_back(tag);
}

TagRecord::TagRecord(uint32_t nonce, const std::string &name) {
    this->nonce = nonce;
    this->name = name;
}

void TagRecord::addBlob(Tfc::BlobRecord *blob) {
    this->blobs.push_back(blob);
}
