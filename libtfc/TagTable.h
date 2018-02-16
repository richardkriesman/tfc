
#ifndef TFC_TAGTABLE_H
#define TFC_TAGTABLE_H

#include <map>

namespace Tfc {

    class TagTableRow {

    public:
        TagTableRow(uint32_t nonce, const std::string &name);

        uint32_t nonce;
        std::string name;

    };

    class TagTable {

    public:
        uint32_t size();
        void add(TagTableRow* row);
        TagTableRow* get(uint32_t nonce);
        TagTableRow* get(std::string name);

        std::map<uint32_t, TagTableRow*>::iterator begin();
        std::map<uint32_t, TagTableRow*>::iterator end();

    private:
        uint32_t _size = 0;
        std::map<std::string, TagTableRow*> nameMap;   // name -> row mapping
        std::map<uint32_t, TagTableRow*> nonceMap; // nonce -> row mapping

    };

}


#endif //TFC_TAGTABLE_H
