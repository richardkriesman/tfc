//
// Created by Richard on 12/6/18.
//

#ifndef TFC_FILE_H
#define TFC_FILE_H

#include <string>

#include <tfc/table.h>

namespace Tfc {

    class File {

    public:
        std::string             getFilename();
        std::uint64_t           getHash();
        std::uint64_t           getSize();
        std::vector<TagRecord*> getTags();

    protected:
        std::string             filename; // the file's name on disk
        uint64_t                hash;     // file hash for verifying integrity
        uint64_t                size;     // the size of the file's contents in bytes
        std::vector<TagRecord*> tags;     // vector of pointers to tags

        uint32_t getNonce();

    private:
        uint32_t nonce; // unique identifier for this record

    };


    class ReadableFile : public Tfc::File {

    };


    class WritableFile : public Tfc::File {

    };

}

#endif //TFC_FILE_H
