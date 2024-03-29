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
#ifndef TFC_EXCEPTION_H
#define TFC_EXCEPTION_H

#include <string>
#include <exception>

namespace Tfc {

    class Exception : public std::exception {

    public:
        explicit Exception(const std::string &message);

        const char *what() const throw() override;

    private:
        std::string message;

    };

}

#endif //TFC_EXCEPTION_H
