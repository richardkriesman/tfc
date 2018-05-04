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
#ifndef TFC_JUMPTABLE_H
#define TFC_JUMPTABLE_H

#include "Record.h"

namespace Tfc {

    class BlobTable {

    public:
        void add(BlobRecord *row);
        BlobRecord *get(uint32_t nonce);
        void remove(BlobRecord* record);
        uint32_t size();

        std::map<uint32_t, BlobRecord *>::iterator begin();
        std::map<uint32_t, BlobRecord *>::iterator end();

    private:
        uint32_t _size = 0;
        std::map<uint32_t, BlobRecord *> map;

    };

};


#endif //TFC_JUMPTABLE_H
