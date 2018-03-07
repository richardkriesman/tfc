
#ifndef TFC_JUMPTABLE_H
#define TFC_JUMPTABLE_H

#include "Record.h"

namespace Tfc {

    class BlobTable {

    public:
        void add(BlobRecord* row);
        BlobRecord* get(uint32_t nonce);
        uint32_t size();

        std::map<uint32_t, BlobRecord*>::iterator begin();
        std::map<uint32_t, BlobRecord*>::iterator end();

    private:
        uint32_t _size = 0;
        std::map<uint32_t, BlobRecord*> map;
    };

}


#endif //TFC_JUMPTABLE_H
