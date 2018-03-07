
#ifndef TFC_TFC_FILE_H
#define TFC_TFC_FILE_H

#include <string>
#include <fstream>
#include <arpa/inet.h>

#include "TfcFileException.h"
#include "BlobTable.h"
#include "TagTable.h"

namespace Tfc {

    enum TfcFileMode {
        CLOSED,
        READ,
        CREATE,
        EDIT
    };

    struct TfcFileBlob {
        uint64_t size;
        char* data;
    };


    class TfcFile {

    public:
        explicit TfcFile(const std::string &filename);

        void mode(TfcFileMode mode);
        TfcFileMode getMode();
        void init();
        uint32_t addBlob(char* bytes, uint64_t size);
        void attachTag(uint32_t nonce, const std::string &tag);
        TfcFileBlob* readBlob(uint32_t nonce);
        std::vector<BlobRecord*> listBlobs();
        std::vector<TagRecord*> listTags();
        bool isEncrypted();
        bool isUnlocked();
        bool doesExist();

    private:

        // file constants
        const uint32_t FILE_VERSION = 1;
        const uint32_t MAGIC_NUMBER = 0xE621126E;

        // header field lengths (in bytes)
        const int MAGIC_NUMBER_LEN = 4;
        const int FILE_VERSION_LEN = 4;
        const int DEK_LEN = 32;
        const int HASH_LEN = 32;
        const int NONCE_LEN = 4;

        // file vars
        TfcFileMode op;           // current operation mode
        std::string filename;     // name of the file
        std::fstream stream;      // file stream
        bool encrypted = false;   // whether the file is encrypted
        bool unlocked = true;     // whether the file is unlocked (true if unencrypted)
        bool exists = false;      // whether the file exists in the filesystem

        // file section byte positions
        std::streampos headerPos;     // start position of header
        std::streampos tagTablePos;   // start position of tag table
        std::streampos blobTablePos;  // start position of blob table
        std::streampos blobListPos;   // start position of blob list

        // next auto-increment table nonces
        uint32_t tagTableNextNonce;   // next nonce for a new tag
        uint32_t blobTableNextNonce;  // next nonce for a new blob

        // in-memory tables
        TagTable* tagTable = nullptr;
        BlobTable* blobTable = nullptr;

        void reset();
        void analyze();
        uint32_t readUInt32();
        uint64_t readUInt64();
        std::string readString();
        void writeUInt32(const uint32_t &value);
        void writeUInt64(const uint64_t &value);
        void writeString(const std::string &value);
        void writeTagTable();
        void writeBlobTable();
        void next(std::streampos length);
        void jump(std::streampos length);
        void jumpBack(std::streampos length);

    };

}

#endif //TFC_TFC_FILE_H
