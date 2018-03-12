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

        std::map<std::string, TagRecord*>::iterator begin();
        std::map<std::string, TagRecord*>::iterator end();

    private:
        uint32_t _size = 0;
        std::map<std::string, TagRecord*> nameMap;   // name -> row mapping
        std::map<uint32_t, TagRecord*> nonceMap;     // nonce -> row mapping

    };

}


#endif //TFC_TAGTABLE_H
