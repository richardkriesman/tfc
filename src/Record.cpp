
#include "Record.h"
#include "TfcFile.h"

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

BlobRecord::BlobRecord(uint32_t nonce, const std::string &name, uint64_t hash, std::streampos start) : Record(nonce)  {
    this->nonce = nonce;
    this->name = name;
    this->hash = hash;
    this->start = start;
}

void BlobRecord::addTag(Tfc::TagRecord* tag) {
    this->tags.push_back(tag);
}

TagRecord::TagRecord(uint32_t nonce, const std::string &name) : Record(nonce) {
    this->nonce = nonce;
    this->name = name;
}

void TagRecord::addBlob(Tfc::BlobRecord *blob) {
    this->blobs.push_back(blob);
}
