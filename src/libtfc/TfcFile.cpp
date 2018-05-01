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
#include <cstring>
#include <chrono>
#include <algorithm>
#include "endian.h"
#include "xxhash/xxhash.h"
#include "libtfc/TfcFile.h"

using namespace Tfc;

/**
 * Creates a new representation of a Tagged File Container (TFC) file, which can be switched between read and write
 * modes.
 *
 * @param filename The path of the file on the disk
 */
TfcFile::TfcFile(const std::string &filename){
    this->op = TfcFileMode::CLOSED;
    this->filename = filename;

    // determine if the file exists
    std::ifstream stream(this->filename);
    this->exists = stream.good();
}

/**
 * Switches the operating mode to CLOSED, READ, CREATE, or EDIT.
 *
 * @param mode The mode to switch to.
 * @throw TfcFileException Failed to open the file in that mode.
 */
void TfcFile::mode(TfcFileMode mode) {
    switch(mode) {
        case TfcFileMode::CLOSED: // close stream and clear flags
            if(this->op == TfcFileMode::CLOSED)
                break;

            this->reset();
            break;
        case TfcFileMode::READ: // open file for reading
            if(this->op == TfcFileMode::READ)
                break;
            if(this->op != TfcFileMode::CLOSED)
                this->reset();

            // open file for reading
            this->stream.open(this->filename, std::ios::in | std::ios::binary);
            if(this->stream.fail())
                throw TfcFileException("Failed to open for reading");
            this->op = TfcFileMode::READ;

            // analyze the file
            this->analyze();

            break;
        case TfcFileMode::CREATE: // open a new file
            if(this->op == TfcFileMode::CREATE)
                break;
            if(this->op != TfcFileMode::CLOSED)
                this->reset();

            // open a new file and truncate it
            this->stream.open(this->filename, std::ios::out | std::ios::binary);
            if(this->stream.fail())
                throw TfcFileException("Failed to open a new file");
            this->op = TfcFileMode::CREATE;
            break;

        case TfcFileMode::EDIT: // open file for editing
            if(this->op == TfcFileMode::EDIT)
                break;
            if(this->op != TfcFileMode::CLOSED)
                this->reset();

            // open file for writing
            this->stream.open(this->filename, std::ios::in | std::ios::out | std::ios::binary);
            if(this->stream.fail())
                throw TfcFileException("Failed to open for editing");
            this->op = TfcFileMode::EDIT;
            break;
        default:
            throw TfcFileException("Invalid mode");
    }
}

/**
 * Returns the current operation mode of the file.
 */
TfcFileMode TfcFile::getMode() {
    return this->op;
}

/**
 * Closes the file stream, resets all flags, and changes the operation mode to CLOSED.
 */
void TfcFile::reset() {
    this->stream.close();
    this->stream.clear();
    this->op = TfcFileMode::CLOSED;
}

/**
 * READ mode operation. Analyzes the structure of the file. Finds the starting position of file sections and builds a
 * blob table for blobs.
 */
void TfcFile::analyze() {
    if(this->op != TfcFileMode::READ)
        throw TfcFileException("File not in READ mode");
    this->jump(0);

    // current state of the analyzer
    enum AnalyzeState {
        HEADER,
        BLOCK_LIST,
        TAG_TABLE,
        BLOB_TABLE,
        END
    };
    int state = AnalyzeState::HEADER;

    // read based on state
    uint32_t magicNumber;
    uint32_t tagCount;
    uint32_t blobCount = 0;
    uint32_t version;
    while(state != AnalyzeState::END) {
        switch (state) {
            case AnalyzeState::HEADER:
                this->headerPos = this->stream.tellg(); // header position

                // check magic number
                magicNumber = this->readUInt32();
                if(magicNumber != MAGIC_NUMBER)
                    throw TfcFileException("Not a valid container file");

                // check file version
                version = this->readUInt32();
                if(version > FILE_VERSION)
                    throw TfcFileException("Container version mismatch. Must be <= " + std::to_string(FILE_VERSION));

                // check if file is encrypted (DEK will be all 0s)
                char dek[32];
                this->stream.read(dek, DEK_LEN);
                for(char byte : dek) {
                    if (byte != 0x0) {
                        this->encrypted = true;
                        this->unlocked = false;
                        break;
                    }
                }

                state++;
                break;
            case AnalyzeState::BLOCK_LIST:
                this->blockListPos = this->stream.tellg();

                // read number of blocks
                this->blockCount = this->readUInt32();

                // skip over the blocks
                this->next(BLOCK_SIZE * this->blockCount);

                state++;
                break;
            case AnalyzeState::TAG_TABLE:
                this->tagTablePos = this->stream.tellg(); // tag table position

                // read next tag nonce
                this->tagTableNextNonce = this->readUInt32();

                // read tag count
                tagCount = this->readUInt32();

                // allocate new tag table (and dealloc any old one)
                delete this->tagTable;
                this->tagTable = new TagTable();

                // build in-memory tag table
                for(uint32_t i = 0; i < tagCount; i++) {

                    // read nonce
                    uint32_t nonce = this->readUInt32();

                    // read name string
                    std::string name = this->readString();

                    // add tag to tag table
                    this->tagTable->add(new TagRecord(nonce, name));

                }

                state++;
                break;
            case AnalyzeState::BLOB_TABLE:
                this->blobTablePos = this->stream.tellg();

                // read next blob nonce
                this->blobTableNextNonce = this->readUInt32();

                // read blob count
                blobCount = this->readUInt32();

                // allocate new blob table (and dealloc any old one)
                delete this->blobTable;
                this->blobTable = new BlobTable();

                // read blob table entries
                for(uint32_t i = 0; i < blobCount; i++) {

                    // get nonce
                    uint32_t nonce = this->readUInt32();

                    // read the name
                    std::string name = this->readString();

                    // read the hash
                    uint64_t hash = this->readUInt64();

                    // get start position
                    uint64_t start = this->readUInt64();

                    // get size
                    uint64_t size = this->readUInt64();

                    // build blob record
                    BlobRecord* blobRecord = new BlobRecord(nonce, name, hash, start, size);

                    // read tag count
                    uint32_t blobTagCount = this->readUInt32();

                    // read in tags
                    for(uint32_t j = 0; j < blobTagCount; j++) {

                        // read tag nonce
                        uint32_t tagNonce = this->readUInt32();

                        // get tag from the tag table
                        TagRecord* tagRecord = this->tagTable->get(tagNonce);
                        if(tagRecord == nullptr) // if tag doesn't exist, just ignore it
                            continue;

                        // link tag and blob together
                        blobRecord->addTag(tagRecord);
                        tagRecord->addBlob(blobRecord);

                    }

                    // add to blob to blob table
                    this->blobTable->add(blobRecord);
                }

                state++;
                break;

            default:
                return;
        }
    }

}

/**
 * Writes out the structure of an empty container file. Overwrites all file data.
 * Must be in CREATE mode.
 *
 * @throw TfcFileException The file is not in CREATE mode.
 */
void TfcFile::init() {
    if(this->op != TfcFileMode::CREATE)
        throw TfcFileException("File not in CREATE mode");
    this->jump(0); // move cursor to beginning of file

    // write header data
    this->writeUInt32(MAGIC_NUMBER); // write magic number
    this->writeUInt32(FILE_VERSION); // write file version

    // write DEK as all 0s (since there's no encryption yet)
    for(int i = 0; i < 8; i++) // 32 * 8 is 256, the size of the DEK
        this->writeUInt32(0x0);

    // write block list - just the count (which is 0) for now
    this->writeUInt32(0);

    // write tag table - for an empty container, this is the next nonce (1) and the count
    this->writeUInt32(1);
    this->writeUInt32(0);

    // write blob table - for an empty container, this is the next nonce (1) and the blob count (0)
    this->writeUInt32(1);
    this->writeUInt32(0);

    // flush the buffer
    this->stream.flush();

    // the file exists now, update state
    this->exists = true;
}

/**
 * Reads a uint32_t from the file and moves the cursor forward by 4 bytes.
 *
 * @return The uint32_t value from the file.
 */
uint32_t TfcFile::readUInt32() {
    uint32_t value;
    this->stream.read(reinterpret_cast<char*>(&value), sizeof(uint32_t));
    if(this->stream.fail())
        throw TfcFileException("Failed to read uint32");
    return ntohl(value);
}

/**
 * Reads a uint64_t from the file and moves the cursor forward by 4 bytes.
 *
 * @return The uint64_t value from the file.
 */
uint64_t TfcFile::readUInt64() {
    uint64_t value;
    this->stream.read(reinterpret_cast<char*>(&value), sizeof(uint64_t));
    if(this->stream.fail())
        throw TfcFileException("Failed to read uint64");
    return be64toh(value);
}

/**
 * Reads a string at the specified position. This function will first read a uint32_t to obtain the length of the
 * string.
 *
 * @return A string at the cursor's current position.
 */
std::string TfcFile::readString() {
    uint32_t length = this->readUInt32(); // length of the string

    // read into buffer
    char buf[length + 1]; // buffer for storing the string + null terminator
    this->stream.read(buf, length);
    if(this->stream.fail())
        throw TfcFileException("Failed to read string");

    // add null terminator at end of buffer
    buf[length] = '\0';

    // convert to string
    return std::string(buf);
}

/**
 * Writes a uint32_t to the file at the current position and moves the cursor forward by 4 bytes.
 *
 * @param value The uint32_t value to write.
 */
void TfcFile::writeUInt32(const uint32_t &value) {
    uint32_t networkByteValue = htonl(value);
    this->stream.write((char*) &networkByteValue, sizeof(uint32_t));
    if(this->stream.fail())
        throw TfcFileException("Failed to write uint32");
}

/**
 * Writes a uint64_t to the file at the current position and moves the cursor forward by 8 bytes.
 *
 * @param value The uint64_t value to write.
 */
void TfcFile::writeUInt64(const uint64_t &value) {
    uint64_t networkByteValue = htobe64(value);
    this->stream.write((char*) &networkByteValue, sizeof(uint64_t));
    if(this->stream.fail())
        throw TfcFileException("Failed to write uint64");
}

/**
 * Writes a string to the file at the current position and moves the cursor forward by a number of bytes equal to the
 * length of the string.
 *
 * @param value The string to write.
 */
void TfcFile::writeString(const std::string &value) {

    // write string length
    this->writeUInt32(static_cast<uint32_t>(value.size()));

    // write string bytes
    this->stream.write(value.c_str(), value.size());
    if(this->stream.fail())
        throw TfcFileException("Failed to write string");
}

/**
 * Moves the cursor forward by a number of bytes.
 *
 * @param length The number of bytes to move forward by.
 */
void TfcFile::next(std::streampos length) {
    this->stream.seekg(length, this->stream.cur);
}

/**
 * Moves the cursor to a number of bytes from the beginning of the file.
 *
 * @param length The number of bytes from the beginning of the file to jump to.
 */
void TfcFile::jump(std::streampos length) {
    this->stream.seekg(length, this->stream.beg);
}

/**
 * Moves the cursor to a number of bytes from the end of the file.
 *
 * @param length The number of bytes from the end of the file to jump to.
 */
void TfcFile::jumpBack(std::streampos length) {
    this->stream.seekg(length, this->stream.end);
}

/**
 * EDIT operation. Adds a blob to the container.
 *
 * @param name The display name of the blob.
 * @param bytes Pointer to the raw bytes of the blob.
 * @param size The size of the blob in bytes.
 * @return The container index that was assigned to the blob.
 */
uint32_t TfcFile::addBlob(const std::string &name, char* bytes, uint64_t size) {
    if(this->op != TfcFileMode::EDIT) // file must be in EDIT mode
        throw TfcFileException("File not in EDIT mode");

    /*
     * Write blob bytes to free blocks
     */
    std::streamsize remainingSize = size;       // number of bytes still left to write
    uint64_t bytePos = 0;                // where we are in the byte array
    std::streampos firstBlockPos;        // position of first block in the block chain
    std::streampos lastBlockNextPos = 0; // position of next pos in last block
    std::streampos blockListDataStart = this->blockListPos // start of block list data section
                                        + static_cast<std::streampos>(BLOCK_LIST_COUNT_SIZE);
    std::streampos selectedBlock = 0;     // currently selected block
    while(remainingSize > 0) { // we still have bytes to write
        bool isFirstBlock = selectedBlock == 0; // first block always starts with selectedBlock == 0

        // find a free block
        this->jump(blockListDataStart); // jump to block list data section start
        char freeBlockBuf[BLOCK_SIZE];  // buffer for storing current block
        while(selectedBlock < blockCount) { // only check existing blocks, we can also make new ones

            // read block into memory
            this->stream.read(freeBlockBuf, BLOCK_SIZE); // read block data and next pos (should be 0 for free blocks)

            // check if block is free
            bool free = true;
            for(char byte : freeBlockBuf) {
                if(byte != 0x0) {
                    free = false;
                    break;
                }
            }
            if(free) { // we found a free block
                this->jump(blockListDataStart + BLOCK_SIZE * selectedBlock); // move back to start of free block
                break;
            }

            // move to next block
            selectedBlock += 1;
            this->jump(blockListDataStart + BLOCK_SIZE * selectedBlock);

        }

        // if first block, record first block's pos
        if(isFirstBlock)
            firstBlockPos = this->stream.tellg();

        // update block count if we're writing to a new block
        bool isNewBlock = selectedBlock >= blockCount;
        if(isNewBlock) {

            // jump to block count
            this->jump(this->blockListPos);

            // write new block count
            this->writeUInt32(++this->blockCount);

            // jump back to beginning of new block
            this->jump(blockListDataStart + BLOCK_SIZE * selectedBlock);

        }

        // determine number of bytes we need to write in this block
        std::streamsize blockDataSize;
        if(remainingSize > BLOCK_DATA_SIZE)
            blockDataSize = BLOCK_DATA_SIZE;
        else
            blockDataSize = remainingSize;

        // write data to block
        this->stream.write(bytes + bytePos, blockDataSize);
        if(this->stream.fail())
            throw TfcFileException("Failed to write blob data");
        bytePos += blockDataSize;

        // update next pos of last block
        if(lastBlockNextPos != 0) { // last block existed

            // jump back to last block's next pos
            this->jump(lastBlockNextPos);

            // write start pos of selected block
            this->writeUInt64(static_cast<uint64_t>(blockListDataStart + BLOCK_SIZE * selectedBlock));

        }

        // go to end of selected block's data section (start of next pos)
        this->jump(blockListDataStart + BLOCK_SIZE * selectedBlock
            + static_cast<std::streampos>(BLOCK_DATA_SIZE));

        // set this block's next pos so the next block will update it
        lastBlockNextPos = this->stream.tellg();

        // zero out next pos
        char nextPosBuf[8] = {};
        this->stream.write(nextPosBuf, BLOCK_NEXT_SIZE);

        // subtract bytes we just wrote from remaining bytes
        remainingSize -= blockDataSize;
        selectedBlock += 1; // so next block we write starts after this block

    }

    /*
     * Rewrite tag table
     */
    this->writeTagTable();

    /*
     * Rewrite blob table with new entry
     */

    // compute a hash of the data
    uint64_t hash = this->hash(bytes, size);

    // create new record in blob table
    auto* record = new BlobRecord(this->blobTableNextNonce++, name, hash, firstBlockPos, size);
    this->blobTable->add(record);

    // rewrite the blob table
    this->writeBlobTable();

    return record->getNonce();
}

/**
 * EDIT operation. Attaches a tag to a blob. If the tag does not exist, it will be created.
 *
 * @param nonce The nonce of the blob to whom the tag will be attached.
 * @param tag The string tag to be attached. Tags are case insensitive.
 */
void TfcFile::attachTag(uint32_t nonce, const std::string &tag) {
    if(this->op != TfcFileMode::EDIT)
        throw TfcFileException("File not in EDIT mode");

    // convert tag to lower case
    std::string tagLower = tag;
    std::transform(tagLower.begin(), tagLower.end(), tagLower.begin(), ::tolower);

    // get the blob from the blob table
    BlobRecord* blobRow = this->blobTable->get(nonce);
    if(blobRow == nullptr)
        throw TfcFileException("No blob was found with ID " + std::to_string(nonce));

    // get the tag from the tag table, or make a new one if it doesn't exist
    TagRecord* tagRow = this->tagTable->get(tagLower);
    if(tagRow == nullptr) { // tag doesn't exist in table, add it

        // add a new tag to the tag table
        tagRow = new TagRecord(this->tagTableNextNonce++, tagLower);
        this->tagTable->add(tagRow);

        // write the tag table to disk
        this->jump(this->tagTablePos);
        this->writeTagTable();

    } else { // tag already exists in tag table

        // check if tag is already attached
        for(auto _tag : blobRow->getTags()) {
            if(_tag == tagRow) // tag is already attached
                throw TfcFileException("Tag is already attached to this blob");
        }

        // jump to blob table
        this->jump(this->blobTablePos);

    }

    // link the blob and tag together
    blobRow->addTag(tagRow);
    tagRow->addBlob(blobRow);

    // write the blob table to disk
    this->writeBlobTable();
}

/**
 * READ operation. Reads a blob with the specified nonce.
 *
 * @param nonce The nonce of the blob to read.
 * @return A TfcFileBlob struct containing the size and char* to the data. Null if the nonce does not exist.
 */
TfcFileBlob* TfcFile::readBlob(uint32_t nonce) {
    if(this->op != TfcFileMode::READ)
        throw TfcFileException("File not in READ mode");

    // get the blob's record
    BlobRecord* record = this->blobTable->get(nonce);
    if(record == nullptr)
        throw TfcFileException("No blob was found with ID " + std::to_string(nonce));

    // create a new blob struct
    auto* blob = new TfcFileBlob();
    blob->record = record;

    // jump to blob file
    std::streampos startPos = record->getStart();
    this->jump(startPos);

    // allocate memory for storing the blob bytes
    blob->data = new char[blob->record->getSize()];

    // read bytes from blocks
    uint64_t remainingSize = blob->record->getSize();
    while(remainingSize > 0) {

        // determine number of bytes to read from block
        uint64_t blockByteCount;
        if(remainingSize > BLOCK_DATA_SIZE)
            blockByteCount = BLOCK_DATA_SIZE;
        else
            blockByteCount = remainingSize;

        // read bytes into the buffer
        this->stream.read(blob->data + (blob->record->getSize() - remainingSize), blockByteCount);
        if(this->stream.fail())
            throw TfcFileException("Failed to read block");

        // subtract bytes we just read from remaining
        remainingSize -= blockByteCount;

        // jump to position of next block
        this->jump(startPos + static_cast<std::streampos>(BLOCK_DATA_SIZE));
        startPos = this->readUInt64();
        this->jump(startPos);

    }

    return blob;

}

/**
 * READ operation. Returns a list of blob table entries.
 *
 * @return A vector of pointers to the entries.
 */
std::vector<BlobRecord*> TfcFile::listBlobs() {
    if(this->op != TfcFileMode::READ)
        throw TfcFileException("File not in READ mode");

    std::vector<BlobRecord*> rows;
    for (auto &iter : *this->blobTable)
        rows.push_back(iter.second);

    return rows;
}

/**
 * READ operation. Returns a list of tag table entries.
 *
 * @return A vector of pointers to the entries.
 */
std::vector<TagRecord*> TfcFile::listTags() {
    if(this->op != TfcFileMode::READ)
        throw TfcFileException("File not in READ mode");

    std::vector<TagRecord*> rows;
    for (auto &iter : *this->tagTable)
        rows.push_back(iter.second);

    return rows;
}

/**
 * READ operation. Given a vector of tag strings, a vector of BlobRecords are returned whose tags match all of the
 * tag strings. This vector will be returned in a descending order of nonces.
 *
 * @param tags A vector of tags
 * @return A vector of BlobRecords whose tags contain all of the tags in the tags parameter.
 */
std::vector<BlobRecord*> TfcFile::intersection(const std::vector<std::string> &tags) {
    if(this->op != TfcFileMode::READ)
        throw TfcFileException("File not in READ mode");

    std::vector<BlobRecord*> result; // set of intersecting BlobRecords

    // build a search set of TagRecords
    std::vector<TagRecord*> searchSet;
    for(const auto &tag : tags) {

        // convert tag to lower case
        std::string tagLower = tag;
        std::transform(tagLower.begin(), tagLower.end(), tagLower.begin(), ::tolower);

        // get tag record
        TagRecord* record = this->tagTable->get(tagLower);
        if(record == nullptr)
            throw TfcFileException(tagLower + " is not a tag");
        searchSet.push_back(record);

    }

    // sort the search set
    std::sort(searchSet.begin(), searchSet.end(), TagRecord::asc);

    // build a union set of blob records (a set of blobs that have at least one of the tags)
    auto* unionSet = new std::vector<BlobRecord*>();
    for(const auto &tagRecord : searchSet) { // for each tag in search set
        std::vector<BlobRecord*> blobs = tagRecord->getBlobs(); // vector of blobs with this tag
        std::sort(blobs.begin(), blobs.end(), BlobRecord::asc);

        // union blob with union set
        auto* newUnionSet = new std::vector<BlobRecord*>(unionSet->size() + blobs.size()); // pointer to the new union set
        std::vector<BlobRecord*>::iterator end;
        end = std::set_union(unionSet->begin(), unionSet->end(), blobs.begin(), blobs.end(), newUnionSet->begin(),
                             BlobRecord::asc);

        // resize new union set
        newUnionSet->resize(static_cast<unsigned long>(end - newUnionSet->begin()));

        // swap in new union set
        auto* oldUnionSet = unionSet;
        unionSet = newUnionSet;
        delete oldUnionSet;
    }

    // intersect each blob in the union set with the search set
    for(const auto &blobRecord : *unionSet) {

        // sort the blob record's tags
        std::vector<TagRecord*> blobTags = blobRecord->getTags();
        std::sort(blobTags.begin(), blobTags.end(), TagRecord::asc);

        // check if the search set's tags are in the blob record's tags
        std::vector<TagRecord*> intersection(searchSet.size());
        std::vector<TagRecord*>::iterator end;
        end = std::set_intersection(searchSet.begin(), searchSet.end(), blobTags.begin(), blobTags.end(),
                                    intersection.begin(), TagRecord::asc);

        // resize the intersection set
        intersection.resize(static_cast<unsigned long>(end - intersection.begin()));

        // if tags match, add this record to the resultant set
        if(intersection.size() == searchSet.size())
            result.push_back(blobRecord);

    }

    delete unionSet;

    return result;

}

/**
 * Whether the file is encrypted.
 */
bool TfcFile::isEncrypted() {
    return this->encrypted;
}

/**
 * Whether the file has been unlocked. If there is no encryption on the file, this will be true.
 */
bool TfcFile::isUnlocked() {
    return this->unlocked;
}

/**
 * Whether the file exists in the filesystem.
 */
bool TfcFile::doesExist() {
    return this->exists;
}

/**
 * Computes a hash from a byte array using the XXH64 variant of the xxHash algorithm.
 *
 * @param bytes The bytes for which the hash will be computed.
 * @param size The size of the bytes
 * @return The computed hash for the bytes.
 */
uint64_t TfcFile::hash(char* bytes, size_t size) {

    // create hash state for storing progress
    XXH64_state_t* state = XXH64_createState();
    if(state == nullptr) // error occurred
        throw TfcFileException("Failed to allocate hash state");

    // allocate buffer
    size_t bufferSize = this->HASH_BUFFER_SIZE; // size of the buffer
    const char* buffer = (char*)malloc(bufferSize);
    if(buffer == nullptr)
        throw TfcFileException("Failed to allocate hash buffer");

    // set the seed for this hash
    const unsigned long long seed = this->MAGIC_NUMBER; // we'll just use the file's magic number as the seed
    const XXH_errorcode resetResult = XXH64_reset(state, seed);
    if(resetResult == XXH_ERROR)
        throw TfcFileException("Failed to seed the hash state");

    // read bytes in blocks
    while(size > 0) {

        // determine how many bytes to read
        size_t byteCount;
        if(size >= this->HASH_BUFFER_SIZE)
            byteCount = this->HASH_BUFFER_SIZE;
        else
            byteCount = size;

        // read bytes into buffer
        std::memcpy(bytes, buffer, byteCount);

        // add buffer to hash
        XXH_errorcode addResult = XXH64_update(state, buffer, byteCount);
        if(addResult == XXH_ERROR)
            throw TfcFileException("Failed to update hash state with block");

        size -= byteCount;
    }

    // compute a hash digest
    uint64_t digest = XXH64_digest(state);

    // clean up
    delete [] buffer;
    XXH64_freeState(state);

    return digest;

}

/**
 * INTERNAL operation. Writes the current tag table from memory to the file at the *current position*. This will update
 * the tag table's position variable. This may overwrite parts of the blob table.
 */
void TfcFile::writeTagTable() {

    // update tag table position
    this->tagTablePos = this->stream.tellg();

    // write next nonce
    this->writeUInt32(this->tagTableNextNonce);

    // write tag count
    this->writeUInt32(this->tagTable->size());

    // rewrite tag table entries
    for(auto &iter : *this->tagTable) {
        TagRecord* row = iter.second;

        // write nonce
        this->writeUInt32(row->getNonce());

        // write name length and name
        this->writeString(row->getName());

    }

    // flush the stream
    this->stream.flush();
}

/**
 * INTERNAL operation. Writes the current blob table from memory to the file at the *current position*. This will update
 * the blob table's position variable.
 */
void TfcFile::writeBlobTable() {

    // update blob table position
    this->blobTablePos = this->stream.tellg();

    // update next nonce
    this->writeUInt32(this->blobTableNextNonce);

    // write blob count
    this->writeUInt32(this->blobTable->size());

    // rewrite blob table entries
    for(auto &iter : *this->blobTable) {
        BlobRecord* row = iter.second;

        // write nonce
        this->writeUInt32(row->getNonce());

        // write name
        this->writeString(row->getName());

        // write hash
        this->writeUInt64(row->getHash());

        // write start pos
        this->writeUInt64(static_cast<uint64_t>(row->getStart()));

        // write size
        this->writeUInt64(row->getSize());

        // write tag count
        this->writeUInt32(static_cast<uint32_t >(row->getTags().size()));

        // write each tag's nonce
        for(auto tag : row->getTags())
            this->writeUInt32(tag->getNonce());
    }

    // flush the stream
    this->stream.flush();
}
