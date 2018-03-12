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
