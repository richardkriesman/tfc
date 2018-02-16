
#ifndef TFC_JUMPTABLE_H
#define TFC_JUMPTABLE_H

#include <map>
#include <cstring>

namespace Tfc {

    class JumpTableRow {

    public:
        JumpTableRow(uint32_t nonce, const char* hash, std::streampos start);

        uint32_t nonce;         // unique identifier for this blob
        char hash[32];          // SHA256 hash
        std::streampos start;   // starting byte position of the blob

    };

    class JumpTable {

    public:
        void add(JumpTableRow* row);
        JumpTableRow* get(uint32_t nonce);
        uint32_t size();

        std::map<uint32_t, JumpTableRow*>::iterator begin();
        std::map<uint32_t, JumpTableRow*>::iterator end();

    private:
        uint32_t _size = 0;
        std::map<uint32_t, JumpTableRow*> map;
    };

}


#endif //TFC_JUMPTABLE_H
