
#include <cstring>
#include <endian.h>
#include "picosha2/picosha2.h"
#include "TfcFile.h"

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
            if(this->op != TfcFileMode::CLOSED)
                this->reset();

            // open a new file and truncate it
            this->stream.open(this->filename, std::ios::out | std::ios::binary);
            if(this->stream.fail())
                throw TfcFileException("Failed to open a new file");
            this->op = TfcFileMode::CREATE;
            break;

        case TfcFileMode::EDIT: // open file for editing
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
 * jump table for blobs.
 */
void TfcFile::analyze() {
    if(this->op != TfcFileMode::READ)
        throw TfcFileException("File not in READ mode");
    this->jump(0);

    // current state of the analyzer
    enum AnalyzeState {
        HEADER,
        BLOB_LIST,
        TAG_TABLE,
        BLOB_TABLE,
        END
    };
    int state = AnalyzeState::HEADER;

    // read based on state
    uint32_t tagCount;
    uint32_t blobCount = 0;
    uint32_t version;
    while(state != AnalyzeState::END) {
        switch (state) {
            case AnalyzeState::HEADER:
                this->headerPos = this->stream.tellg(); // header position

                // skip to tag table
                this->stream.seekg(MAGIC_NUMBER_LEN);

                // check file version
                version = this->readUInt32();
                if(version > FILE_VERSION)
                    throw TfcFileException("tfc: Container version mismatch. Must be <= " + FILE_VERSION);

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
            case AnalyzeState::BLOB_LIST:
                this->blobListPos = this->stream.tellg();

                // read number of blobs
                blobCount = this->readUInt32();

                // skip over the blobs
                for(uint32_t i = 0; i < blobCount; i++) {
                    uint64_t size = this->readUInt64(); // size of the blob
                    this->next(size); // skip over blob bytes
                }

                state++;
                break;
            case AnalyzeState::TAG_TABLE:
                this->tagTablePos = this->stream.tellg(); // tag table position

                // read next tag nonce
                this->tagTableNextNonce = this->readUInt32();

                // read tag count
                tagCount = this->readUInt32();

                // skip over tags
                for(uint32_t i = 0; i < tagCount; i++) {

                    // skip nonce
                    this->next(NONCE_LEN);

                    // read name length and skip over name
                    uint32_t nameLen = this->readUInt32();
                    this->next(nameLen);

                }

                state++;
                break;
            case AnalyzeState::BLOB_TABLE:
                this->blobTablePos = this->stream.tellg();

                // read next blob nonce
                this->blobTableNextNonce = this->readUInt32();

                // allocate new jump table (and dealloc any old one)
                delete this->jumpTable;
                this->jumpTable = new JumpTable();

                // read blob table entries
                for(uint32_t i = 0; i < blobCount; i++) {

                    // get nonce
                    uint32_t nonce = this->readUInt32();

                    // read the hash
                    char hash[HASH_LEN];
                    this->stream.read(hash, sizeof(hash));

                    // get start position
                    uint64_t start = this->readUInt64();

                    // skip over tags, we don't care about them at the moment
                    uint32_t blobTagCount = this->readUInt32();
                    this->next(sizeof(uint32_t) * blobTagCount);

                    // add to blob to jump table
                    this->jumpTable->add(new JumpTableRow(nonce, hash, start));
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
    for(int i = 0; i < 8; i++) { // 32 * 8 is 256, the size of the DEK
        this->writeUInt32(0x0);
    }

    // write blob list - just the count (which is 0) for now
    this->writeUInt32(0);

    // write tag table - for an empty container, this is the next nonce (1) and the count
    this->writeUInt32(1);
    this->writeUInt32(0);

    // write blob table - for an empty container, this is the next nonce (1)
    this->writeUInt32(1);

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
 * @param bytes Pointer to the raw bytes of the blob.
 * @param size The size of the blob in bytes.
 * @return The container index that was assigned to the blob.
 */
uint32_t TfcFile::addBlob(char *bytes, uint64_t size) {
    if(this->op != TfcFileMode::EDIT) // file must be in EDIT mode
        throw TfcFileException("File not in EDIT mode");

    /*
     * Write blob bytes
     */

    // update blob count at beginning of blob list
    this->jump(this->blobListPos);
    this->writeUInt32(this->jumpTable->size() + 1);

    // jump to beginning of tag table (where the new blob will be written)
    this->jump(this->tagTablePos);
    auto blobStartPos = static_cast<uint64_t>(this->stream.tellg());

    // write blob size
    this->writeUInt64(static_cast<uint64_t>(size));

    // write bytes to file
    this->stream.write(bytes, size);
    if(this->stream.fail())
        throw TfcFileException("Failed to write blob data");

    /*
     * Rewrite tag table
     */
    // TODO: Actual tag table rewriting will be needed, but tags haven't been implemented yet

    // update tag table position
    this->tagTablePos = this->stream.tellg();

    // write next nonce
    this->writeUInt32(this->tagTableNextNonce);

    // write tag count
    this->writeUInt32(0);

    /*
     * Rewrite blob table with new entry
     */

    // update blob table position
    this->blobTablePos = this->stream.tellg();

    // update next nonce
    this->writeUInt32(this->blobTableNextNonce + 1);

    // rewrite blob table entries
    JumpTableList* list = this->jumpTable->list();
    for(uint32_t i = 0; i < list->count; i++) {

        // write nonce
        this->writeUInt32(list->rows[i]->nonce);

        // write sha256 hash
        this->stream.write(reinterpret_cast<char*>(list->rows[i]->hash), HASH_LEN);
        if(this->stream.fail())
            throw TfcFileException("Failed to rewrite hash for " + list->rows[i]->nonce);

        // write start pos
        this->writeUInt64(static_cast<uint64_t>(list->rows[i]->start));

        // TODO: write tags - for now, the count is just 0
        this->writeUInt32(0);

    }
    delete list;

    /*
     * Write blob table entry
     */

    // write the nonce
    this->writeUInt32(this->blobTableNextNonce);

    // calculate and write sha256 hash
    std::vector<unsigned char> hash(32);
    picosha2::hash256(bytes, bytes + size, hash.begin(), hash.end());
    this->stream.write(reinterpret_cast<char*>(hash.data()), 32);
    if(this->stream.fail())
        throw TfcFileException("Failed to write blob hash");

    // write start pos and tag count
    this->writeUInt64(blobStartPos); // write start pos
    this->writeUInt32(0); // write tag count (0 for new files)

    // flush buffer to disk
    this->stream.flush();
    return this->blobTableNextNonce++;
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

    // try to get the blob table entry
    if(nonce > this->jumpTable->size())
        return nullptr;

    // create a new blob struct
    auto* blob = new TfcFileBlob();

    // jump to blob file
    JumpTableRow* row = this->jumpTable->get(nonce);
    if(row == nullptr)
        throw TfcFileException("ID not not found");
    auto startPos = static_cast<uint64_t>(row->start);
    this->jump(startPos);
    delete row;

    // read blob length
    blob->size = this->readUInt64();

    // allocate memory for storing the blob bytes
    blob->data = new char[blob->size];

    // copy bytes into the byte array
    this->stream.read(blob->data, blob->size);

    return blob;

}

/**
 * READ operation. Returns a list of blob table entries.
 *
 * @return A pointer to a JumpTableList containing the count and an array of pointers to the entries.
 */
JumpTableList* TfcFile::listBlobs() {
    if(this->op != TfcFileMode::READ)
        throw TfcFileException("File not in READ mode");

    return this->jumpTable->list();
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
