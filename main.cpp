#include <iostream>
#include <vector>
#include <sstream>
#include "libtfc/TfcFile.h"
#include "Colors.h"

const std::string VERSION = "0.0.1"; // NOLINT

void help();
uint32_t stash(Tfc::TfcFile* file, const std::string &filename);
void unstash(Tfc::TfcFile* file, uint32_t id, const std::string &filename);
std::vector<std::string> parseInput(const std::string &input);
std::string status(bool success = true);

int main(int argc, char** argv) {

    // check for proper number of arguments
    if(argc < 2) {
        help();
        return 1;
    }
    std::string filename = argv[1]; // filename, may also be --help or --version

    // check for version or help flags
    if(filename == "--help") { // help flag
        help();
        return 0;
    } else if(filename == "--version") { // version flag
        std::cout << "Tagged File Containers (TFC) v" << VERSION << "\n";
        return 0;
    }

    // try to open a file
    Tfc::TfcFile* file = nullptr;
    try {
        std::string extension(".tfc");
        file = new Tfc::TfcFile(filename + extension);
    } catch (Tfc::TfcFileException &ex) {
        std::cerr << "tfc: " << ex.what() << "\n";
        return 1;
    }

    // print encryption warning if file is not encrypted
    if(!file->isEncrypted() && file->doesExist()) {
        std::cout << Colors::BOLD << Colors::YELLOW << "Warning: " << Colors::RESET;
        std::cout << "This container is not encrypted. Files can be unstashed by anyone!\n";
    }

    // handle commands
    std::string input;
    while(input != "exit") {

        // print prompt
        if(file->isUnlocked() && file->doesExist())
            std::cout << Colors::GREEN;
        else
            std::cout << Colors::GREY;
        std::cout << "tfc> " << Colors::RESET;

        // get input
        std::cin >> input;
        std::vector<std::string> commands = parseInput(input);

        // process input
        try {

            // help command
            if (commands[0] == "help") {
                help();
                continue;
            }

            // init command
            if (commands[0] == "init") {
                file->mode(Tfc::TfcFileMode::CREATE);
                file->init();
                file->mode(Tfc::TfcFileMode::CLOSED);
                std::cout << status(true) << " Created container file at " << filename << "\n";
                continue;
            }

            // list command
            if(commands[0] == "list") {
                file->mode(Tfc::TfcFileMode::READ);
                Tfc::JumpTableList *list = file->listBlobs();
                for (uint32_t i = 0; i < list->count; i++) {
                    printf("%10d\t", list->rows[i]->nonce); // print nonce

                    // print hash
                    for (char j : list->rows[i]->hash) {
                        unsigned char byte = 0x00;
                        byte |= j;

                        printf("%02X ", (unsigned int) (byte & 0xFF));
                    }

                    printf("\n");
                }
                continue;
            }

            // stash command
            if (commands[0] == "stash" && commands.size() == 2) {
                uint32_t nonce = stash(file, commands[1]);

                std::cout << status(true) << " Stashed " << commands[1] << " with ID " << nonce << "\n";
                continue;
            }

            // unstash command
            if (commands[0] == "unstash" && commands.size() == 3) {
                int32_t nonce = std::stoi(commands[1]);
                if (nonce < 0)
                    throw Tfc::TfcFileException("File IDs cannot be negative");
                unstash(file, static_cast<uint32_t>(nonce), commands[2]);

                std::cout << status(true) << " Unstashed " << nonce << " into " << commands[2];
                continue;
            }

            // unknown command
            if(commands[0] != "exit")
                std::cerr << status(false) << " Invalid command. Type \"help\" for a list of commands.\n";

        } catch (Tfc::TfcFileException &ex) {
            std::cerr << status(false) << " " << ex.what() << "\n";
        } catch (std::exception &ex) {
            std::cerr << status(false) << " " << ex.what() << "\n";
        }

    }

    // clean up
    delete file;

    return 0;

}

/**
 * Reads a file from the filesystem
 *
 * @param file The TFC file object
 * @param filename The path of the file to stash
 * @return The ID that was assigned to the stashed file.
 */
uint32_t stash(Tfc::TfcFile* file, const std::string &filename) {

    // read in file
    std::ifstream stream;
    stream.open(filename, std::ios::binary | std::ios::in | std::ios::ate);
    if(stream.fail())
        throw Tfc::TfcFileException("Failed to open file " + filename + " for reading");

    // read stream size
    std::streamsize size = stream.tellg();
    stream.seekg(0, stream.beg);

    // read data
    auto* data = new char[size];
    stream.read(data, size);
    stream.close();

    // add data as blob
    file->mode(Tfc::TfcFileMode::READ);
    file->mode(Tfc::TfcFileMode::EDIT);
    uint32_t nonce = file->addBlob(data, static_cast<uint64_t>(size));
    file->mode(Tfc::TfcFileMode::CLOSED);

    return nonce;
}

void unstash(Tfc::TfcFile* file, uint32_t id, const std::string &filename) {

    // read the blob from the container
    file->mode(Tfc::TfcFileMode::READ);
    Tfc::TfcFileBlob* blob = file->readBlob(id);
    file->mode(Tfc::TfcFileMode::CLOSED);
    if(blob == nullptr)
        throw Tfc::TfcFileException("No file with that ID exists");

    // open the file for writing
    std::ofstream stream(filename, std::ios::out | std::ios::binary);
    if(stream.fail())
        throw Tfc::TfcFileException("Failed to open file " + filename + " for writing");
    stream.write(blob->data, blob->size);
    stream.close();

    delete blob;

}

/**
 * Prints help text
 */
void help() {
    printf("Tagged File Containers\n\n"
                   "Usage: tfc <filename>\n"
                   "\t%-25s\tprints the version\n"
                   "\t%-25s\tprints this help page\n\n"
                   "Commands:\n"
                   "\t%-25s\tcreates a new container file\n"
                   "\t%-25s\tcopies a file into the container\n"
                   "\t%-25s\tcopies a file out of the container\n"
                   "\t%-25s\tdeletes a file from the container (TBI)\n"
                   "\t%-25s\tadds a tag to a file (TBI)\n"
                   "\t%-25s\tremoves a tag from a file (TBI)\n"
                   "\t%-25s\tsearches for files matching all of the tags (TBI)\n"
                   "\t%-25s\tlists all files by their ID and hash\n",
    "--version", "--help", "init", "stash <filename>", "unstash <id> <filename>", "delete <id>",
           "tag <id> <tag>", "untag <id> <tag>", "search <tag> ...", "list");
}

/**
 * Helper function. Splits a string by spaces unless escaped with \ or encapsulated with ".
 *
 * @param input The string to be split.
 * @param delimiter The delimiter to split by.
 * @return A vector of tokens split by the delimiter.
 */
std::vector<std::string> parseInput(const std::string &input) {
    std::vector<std::string> tokens;    // the vector of tokens
    std::istringstream stream(input);   // stream containing the string input
    std::string token;                  // the current token
    bool isEncapsulated = false;        // whether the token is encapsulated between quotation marks
    bool isEscaped = false;             // whether the current character is escaped

    while(true) {

        // get char from stream
        char c;
        stream.get(c);
        if(stream.eof()) // end of file, stop processing
            break;

        // set escaped status is backslash
        if(c == '\\' && !isEscaped) {
            isEscaped = true;
            continue;
        }

        // set encapsulation status if quotation mark
        if(c == '"' && !isEscaped) {
            isEncapsulated = !isEncapsulated;
            continue;
        }

        // end token if space (and not escaped or encapsulated)
        if(c == ' ' && !isEscaped && !isEncapsulated) {
            tokens.push_back(token);
            token = "";
        }

        // add char to token
        token += c;
        isEscaped = false;

    }

    // add remaining token to vector
    tokens.push_back(token);

    return tokens;
}

/**
 * Returns a string with a colored status indicator based on whether a command succeeded or failed.
 */
std::string status(bool success) {
    std::string statusStr = "[" + Colors::BOLD;
    if(success)
        statusStr += Colors::GREEN + Colors::CHECKMARK;
    else
        statusStr += Colors::RED + Colors::CROSSMARK;
    statusStr += Colors::RESET + "]";
    return statusStr;
}