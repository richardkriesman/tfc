
#include <cstring>
#include <endian.h>
#include "picosha2/picosha2.h"
#include "TfcFile.h"

/**
 * Creates a new representation of a Tagged File Container (TFC) file, which can be switched between read and write
 * modes.
 *
 * @param filename The path of the file on the disk
 */
TfcFile::TfcFile(const std::string &filename){
    this->op = TfcFileMode::CLOSED;
    this->filename = filename;
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

            //

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
        TAG_TABLE,
        BLOB_TABLE,
        BLOB_LIST,
        END
    };
    int state = AnalyzeState::HEADER;

    // read based on state
    uint32_t tagCount;
    while(state != AnalyzeState::END) {
        switch (state) {
            case AnalyzeState::HEADER:
                this->headerPos = this->stream.tellg(); // header position

                // skip to tag table
                this->stream.seekg(MAGIC_NUMBER_LEN + FILE_VERSION_LEN + DEK_LEN, this->stream.beg);

                state++;
                break;
            case AnalyzeState::TAG_TABLE:
                this->tagTablePos = this->stream.tellg(); // tag table position

                // read tag count
                tagCount = this->readUInt32();

                // skip over tags
                for(uint32_t i = 0; i < tagCount; i++) {

                    // read name length and skip over name
                    uint32_t nameLen = this->readUInt32();
                    this->next(nameLen);

                }

                state++;
                break;
            case AnalyzeState::BLOB_TABLE:
                this->blobTablePos = this->stream.tellg();

                // read number of blobs
                this->blobCount = this->readUInt32();

                // alloc jump table memory
                delete this->blobJumps;
                this->blobJumps = new std::streampos[this->blobCount];

                // read blob table entries
                for(uint32_t i = 0; i < this->blobCount; i++) {
                    uint32_t startPos = this->readUInt32();

                    // skip over tags, we don't care about them at the moment
                    uint32_t blobTagCount = this->readUInt32();
                    this->next(sizeof(uint32_t) * blobTagCount);

                    // add to jump table
                    this->blobJumps[i] = startPos;
                }

                state++;
                break;
            case AnalyzeState::BLOB_LIST:
                this->blobListPos = this->stream.tellg();

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

    // write tag table - for an empty container, this is just the count
    this->writeUInt32(0);

    // write blob table - for an empty container, this is just the blob count
    this->writeUInt32(0);

    // nothing to write for the blob list, just flush the buffer
    this->stream.flush();
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
uint32_t TfcFile::addBlob(char *bytes, std::streamsize size) {
    if(this->op != TfcFileMode::EDIT) // file must be in EDIT mode
        throw TfcFileException("File not in EDIT mode");

    // jump to blob table
    this->jump(this->blobTablePos);

    // update blob count
    this->writeUInt32(++this->blobCount);

    // iterate through blob table entries (also get the start pos of the last blob)
    for(uint32_t i = 0; i < this->blobCount - 1; i++) {

        // skip over start pos
        this->next(sizeof(uint64_t));

        // skip over tags
        uint32_t tagCount = this->readUInt32();
        this->next(sizeof(uint32_t) * tagCount);
    }

    // get end of file position from start
    std::streampos currentPos = this->stream.tellg();
    this->jumpBack(0);
    auto newBlobStart = static_cast<uint64_t>(this->stream.tellg());
    this->stream.seekg(currentPos);

    // create new blob table entry
    this->writeUInt64(newBlobStart); // write start pos
    this->writeUInt32(0); // write tag count (0 for new files)

    // jump to start pos of the last blob and skip over last blob
    this->stream.seekg(newBlobStart);
    if(this->blobCount - 1 > 0) {
        this->next(HASH_LEN); // skip over sha256 hash

        // read byte length and skip over that
        uint64_t byteLen = this->readUInt64();
        this->next(byteLen);
    }

    // calculate and write sha256 hash for file
    std::vector<unsigned char> hash(32);
    picosha2::hash256(bytes, bytes + size, hash.begin(), hash.end());
    this->stream.write(reinterpret_cast<char*>(hash.data()), 32);
    if(this->stream.fail())
        throw TfcFileException("Failed to write blob hash");

    // write blob size
    this->writeUInt64(static_cast<uint64_t>(size));

    // write bytes to file
    this->stream.write(bytes, size);
    if(this->stream.fail())
        throw TfcFileException("Failed to write blob data");

    // flush buffer to disk
    this->stream.flush();
}

/**
 * Creates a new TfcFileException, for errors related to TfcFile.
 *
 * @param message A message detailing the error.
 */
TfcFileException::TfcFileException(const std::string &message) {
    this->message = message;
}

/**
 * Returns a message detailing the error.
 */
const char* TfcFileException::what() const throw() {
    return this->message.c_str();
}