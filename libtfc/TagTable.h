
#ifndef TFC_TAGTABLE_H
#define TFC_TAGTABLE_H

#include <map>
#include <vector>
#include "Record.h"

namespace Tfc {

    class TagTable {

    public:
        uint32_t size();
        void add(TagRecord* row);
        TagRecord* get(uint32_t nonce);
        TagRecord* get(std::string name);

        std::map<uint32_t, TagRecord*>::iterator begin();
        std::map<uint32_t, TagRecord*>::iterator end();

    private:
        uint32_t _size = 0;
        std::map<std::string, TagRecord*> nameMap;   // name -> row mapping
        std::map<uint32_t, TagRecord*> nonceMap;     // nonce -> row mapping

    };

}


#endif //TFC_TAGTABLE_H
