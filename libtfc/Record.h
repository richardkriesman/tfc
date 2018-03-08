
#ifndef TFC_RECORD_H
#define TFC_RECORD_H

#include <map>
#include <cstring>
#include <vector>

namespace Tfc {

    class TagRecord; // tag class pre-declaration;

    class BlobRecord {

    public:
        BlobRecord(uint32_t nonce, std::string name, const char *hash, std::streampos start);

        bool operator<(BlobRecord* other);

        uint32_t getNonce() { return this->nonce; }
        std::string getName() { return this->name; }
        char *getHash() { return this->hash; }
        std::streampos getStart() { return this->start; }
        std::vector<Tfc::TagRecord*> getTags() { return this->tags; }
        void addTag(Tfc::TagRecord* tag);

    private:
        uint32_t nonce;                     // unique identifier for this blob
        std::string name;                   // original file name
        char hash[32];                      // SHA256 hash
        std::streampos start;               // starting byte position of the blob
        std::vector<Tfc::TagRecord* > tags; // vector of tag pointers

        friend class BlobTable;

    };

    class TagRecord {

    public:
        TagRecord(uint32_t nonce, const std::string &name);

        bool operator<(TagRecord* other);

        const uint32_t getNonce() { return this->nonce; }
        const std::string getName() { return this->name; }
        const std::vector<Tfc::BlobRecord*> getBlobs() { return this->blobs; }

        void addBlob(Tfc::BlobRecord* blob);

    private:
        uint32_t nonce;
        std::string name;

        std::vector<Tfc::BlobRecord*> blobs;

        friend class TagTable;

    };

}


#endif //TFC_RECORD_H
