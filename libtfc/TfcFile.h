
#ifndef TFC_TFC_FILE_H
#define TFC_TFC_FILE_H

#include <string>
#include <fstream>
#include <arpa/inet.h>

enum TfcFileMode {
    CLOSED,
    READ,
    CREATE,
    EDIT
};


class TfcFile {

public:
    explicit TfcFile(const std::string &filename);

    void mode(TfcFileMode mode);
    TfcFileMode getMode();

    void init();
    uint32_t addBlob(char* bytes, std::streamsize size);

private:

    // file constants
    const uint32_t FILE_VERSION = 1;
    const uint32_t MAGIC_NUMBER = 0xE621126E;

    // header field lengths (in bytes)
    const int MAGIC_NUMBER_LEN = 4;
    const int FILE_VERSION_LEN = 4;
    const int DEK_LEN = 32;
    const int HASH_LEN = 32;

    TfcFileMode op;         // current operation mode
    std::string filename;   // name of the file
    std::fstream stream;    // file stream

    std::streampos headerPos;     // start position of header
    std::streampos tagTablePos;   // start position of tag table
    std::streampos blobTablePos;  // start position of blob table
    std::streampos blobListPos;   // start position of blob list

    uint32_t blobCount;                   // number of blobs in the file
    std::streampos* blobJumps = nullptr;  // dynamic map of blob index to byte position

    void reset();
    void analyze();

    uint32_t readUInt32();
    uint64_t readUInt64();
    void writeUInt32(const uint32_t &value);
    void writeUInt64(const uint64_t &value);
    void next(std::streampos length);
    void jump(std::streampos length);
    void jumpBack(std::streampos length);
};


class TfcFileException : public std::exception {

public:
    explicit TfcFileException(const std::string &message);
    const char* what() const throw() override;

private:
    std::string message;

};


#endif //TFC_TFC_FILE_H
