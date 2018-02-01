#include "TfcFileException.h"

using namespace Tfc;

/**
 * Creates a new TfcFileException, for errors related to TfcFile.
 *
 * @param message A message detailing the error.
 */
TfcFileException::TfcFileException(const std::string &message) {
    this->message = message;
}

/**
 * Returns a message detailing the error.
 */
const char* TfcFileException::what() const throw() {
    return this->message.c_str();
}