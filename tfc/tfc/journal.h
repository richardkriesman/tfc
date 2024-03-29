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
#ifndef TFC_JOURNAL_H
#define TFC_JOURNAL_H

#include <string>
#include <fstream>

namespace Tfc {

    enum JournalState {
        PENDING,
        IN_PROGRESS,
        COMMITTED,
        ERROR
    };

    class Journal {

    public:
        explicit Journal(const std::string &filename);

    private:
        std::string filename;
        bool exists;
        std::fstream stream;

    };

}


#endif //TFC_JOURNAL_H
