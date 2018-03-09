
#ifndef TFC_RECORD_H
#define TFC_RECORD_H

#include <map>
#include <cstring>
#include <vector>

namespace Tfc {

    // pre-declarations
    class TagRecord;

    // record base class
    class Record {

    public:
        explicit Record(uint32_t nonce);

        // ordering functions
        static bool asc(Record* record1, Record* record2);
        static bool desc(Record* record1, Record* record2);

        // accessors
        uint32_t getNonce() { return this->nonce; }

    protected:
        uint32_t nonce; // unique identifier for this record

    };

    class BlobRecord : public Record {

    public:
        BlobRecord(uint32_t nonce, const std::string &name, uint64_t hash, std::streampos start);

        std::string getName() { return this->name; }
        uint64_t getHash() { return this->hash; }
        std::streampos getStart() { return this->start; }
        std::vector<Tfc::TagRecord*> getTags() { return this->tags; }
        void addTag(Tfc::TagRecord* tag);

    private:
        std::string name;                   // original file name
        uint64_t hash;                      // file hash
        std::streampos start;               // starting byte position of the blob
        std::vector<Tfc::TagRecord* > tags; // vector of tag pointers

        friend class BlobTable;

    };

    class TagRecord : public Record {

    public:
        TagRecord(uint32_t nonce, const std::string &name);

        const std::string getName() { return this->name; }
        const std::vector<Tfc::BlobRecord*> getBlobs() { return this->blobs; }

        void addBlob(Tfc::BlobRecord* blob);

    private:
        std::string name;
        std::vector<Tfc::BlobRecord*> blobs;

        friend class TagTable;

    };

}


#endif //TFC_RECORD_H
