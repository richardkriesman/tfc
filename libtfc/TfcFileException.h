
#ifndef TFC_TFCFILEEXCEPTION_H
#define TFC_TFCFILEEXCEPTION_H

#include <string>
#include <exception>

namespace Tfc {

    class TfcFileException : public std::exception {

    public:
        explicit TfcFileException(const std::string &message);

        const char *what() const throw() override;

    private:
        std::string message;

    };

}

#endif //TFC_TFCFILEEXCEPTION_H
