#include <iostream>
#include <vector>
#include <sstream>
#include <unistd.h>
#include "TfcFile.h"
#include "Terminal.h"
#include "Task.h"

void await(Task &task, const std::string &message);
void help();
std::string join(const std::vector<std::string> &strings, const std::string &delim);
std::vector<std::string> parseInput(const std::string &input);
void printBlobs(const std::vector<Tfc::BlobRecord*> &blobs);
std::vector<std::string> split(const std::string &string, char delim);
uint32_t stash(Tfc::TfcFile* file, const std::string &filename, const std::string &path);
std::string status(bool success = true);
Tfc::BlobRecord* unstash(Tfc::TfcFile* file, uint32_t id, const std::string &filename = "");

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
        std::cout << Terminal::Decorations::BOLD << Terminal::Colors::YELLOW << "Warning: "
                  << Terminal::Decorations::RESET;
        std::cout << "This container is not encrypted. Files can be unstashed by anyone!\n";
    }

    // handle commands
    std::string input;
    while(input != "exit") {

        // print prompt
        if(file->isUnlocked() && file->doesExist())
            std::cout << Terminal::Colors::GREEN;
        else
            std::cout << Terminal::Colors::GREY;
        std::cout << "tfc> " << Terminal::Decorations::RESET;

        // get input
        std::getline(std::cin, input);
        std::vector<std::string> commands = parseInput(input);

        // process input
        try {

            // help command
            if (commands[0] == "help") {
                help();
                continue;
            }

            // about command
            if (commands[0] == "about") {
                std::cout << Terminal::Decorations::BOLD <<  "Tagged File Containers (TFC)\n\n"
                          << Terminal::Decorations::RESET
                          << "Copyright © Richard Kriesman 2018.\n"
                             "https://richardkriesman.com\n\n"
                             "The TFC client and libtfc library are released under the MIT License.\n"
                             "You are free to share and modify this software.\n";

                continue;
            }

            // clear screen command
            if (commands[0] == "clear") {
                std::cout << Terminal::Screen::CLEAR << status(true) << " Cleared screen\n";
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

            // list files command
            if(commands[0] == "files") {
                file->mode(Tfc::TfcFileMode::READ);
                printBlobs(file->listBlobs());
                file->mode(Tfc::TfcFileMode::CLOSED);
                continue;
            }

            // list tags command
            if(commands[0] == "tags") {
                file->mode(Tfc::TfcFileMode::READ);

                printf("%-10s\t%-10s\n", "Name", "File Count");
                printf("%-10s\t%-10s\n", "----------", "----------");

                for(Tfc::TagRecord* row : file->listTags())
                    printf("%-10s\t%-10lu\n", row->getName().c_str(), row->getBlobs().size());

                file->mode(Tfc::TfcFileMode::CLOSED);
                continue;
            }

            // stash command
            if (commands[0] == "stash" && commands.size() == 2) {

                // determine the name of the file
                std::vector<std::string> path = split(commands[1], '/');
                std::string name = path[path.size() - 1];

                // stash the file
                Task task([&file, &name, &commands]() -> void* {
                    auto* nonce = new uint32_t();
                    *nonce = stash(file, name, commands[1]);

                    return static_cast<void*>(nonce);
                });

                // wait for the task to complete
                task.schedule();
                await(task, "Stashing " + name);
                if (task.getState() == TaskState::FAILED)
                    throw task.getException();

                // extract the nonce
                auto* nonce = static_cast<uint32_t*>(task.getResult());

                std::cout << status(true) << " Stashed " << name << " with ID " << *nonce << "\n";
                delete nonce;
                continue;
            }

            // unstash command
            if (commands[0] == "unstash" && (commands.size() == 2 || commands.size() == 3)) {
                int32_t nonce = std::stoi(commands[1]);
                if (nonce < 0)
                    throw Tfc::TfcFileException("File IDs cannot be negative");

                // unstash the file
                Task task([&file, nonce, &commands]() -> void* {
                    auto* name = new std::string();

                    if(commands.size() == 2) { // use original filename
                        Tfc::BlobRecord* result = unstash(file, static_cast<uint32_t>(nonce));
                        *name = result->getName();
                    } else { // use explicit filename
                        unstash(file, static_cast<uint32_t>(nonce), commands[2]);
                        *name = commands[2];
                    }

                    return static_cast<void*>(name);
                });

                // wait for the task to complete
                task.schedule();
                await(task, "Unstashing file");
                if (task.getState() == TaskState::FAILED)
                    throw task.getException();

                // extract the name
                auto* name = static_cast<std::string*>(task.getResult());

                // output success message
                std::cout << status(true) << " Unstashed " << nonce << " into " << *name << "\n";
                delete name;
                continue;
            }

            // tag command
            if (commands[0] == "tag" && commands.size() >= 3) {
                int32_t nonce = std::stoi(commands[1]);
                if (nonce < 0)
                    throw Tfc::TfcFileException("File IDs cannot be negative");

                // attach the tags
                file->mode(Tfc::TfcFileMode::READ);
                file->mode(Tfc::TfcFileMode::EDIT);
                for(int i = 2; i < commands.size(); i++) {
                    file->attachTag(static_cast<uint32_t>(nonce), commands[i]);
                    std::cout << status(true) << " Tagged " << nonce << " as " << commands[i] << "\n";
                }
                file->mode(Tfc::TfcFileMode::CLOSED);

                continue;
            }

            // search command
            if (commands[0] == "search" && commands.size() > 1) {

                // build a set of tags
                std::vector<std::string> tags;
                for(int i = 1; i < commands.size(); i++)
                    tags.push_back(commands[i]);

                // get a set of intersecting blobs
                std::vector<Tfc::BlobRecord*> intersection;
                file->mode(Tfc::TfcFileMode::READ);
                intersection = file->intersection(tags);
                file->mode(Tfc::TfcFileMode::CLOSED);

                // print intersecting blobs
                printBlobs(intersection);

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
 * Blocks the current thread, showing a spinner and message to the user while a Task runs.
 *
 * @param task The task to await.
 * @param message A message to display to the user explaining the operation.
 */
void await(Task &task, const std::string &message) {
    char states[] = { '-', '\\', '|', '/' }; // animation state
    int i = 0; // current animation state

    // print message
    std::cout << "[" << Terminal::Decorations::BOLD << Terminal::Colors::YELLOW << states[i]
              << Terminal::Decorations::RESET << "] " << message;

    // animate spinner until task is done
    TaskState state = task.getState();
    while(state != TaskState::COMPLETED && state != TaskState::FAILED) {

        // update animation state
        std::cout << Terminal::Cursor::SAVE << Terminal::Cursor::HOME << Terminal::Cursor::forward(1)
                  << Terminal::Decorations::BOLD << Terminal::Colors::YELLOW << states[i]
                  << Terminal::Decorations::RESET << Terminal::Cursor::RESTORE << std::flush;

        // move to next animation state
        if(i < 3)
            i++;
        else
            i = 0;

        // wait a little bit before trying again
        usleep(75000);
        state = task.getState();

    }

    // delete the "in progress line"
    std::cout << Terminal::Cursor::HOME << Terminal::Cursor::ERASE_EOL << std::flush;
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
                   "\t%-25s\tprints this help page\n"
                   "\t%-25s\tdisplays copyright information\n"
                   "\t%-25s\tclears the screen\n"
                   "\t%-25s\tcreates a new unencrypted container file\n"
                   "\t%-25s\tconfigures encryption on this container\n"
                   "\t%-25s\tcopies a file into the container\n"
                   "\t%-25s\tcopies a file out of the container\n"
                   "\t%-25s\tdeletes a file from the container\n"
                   "\t%-25s\tadds a tag to a file\n"
                   "\t%-25s\tremoves a tag from a file\n"
                   "\t%-25s\tsearches for files matching the tags\n"
                   "\t%-25s\tlists all files with their ID and tags\n"
                   "\t%-25s\tlists all tags by their name\n",
           "--version", "--help", "help", "about", "clear", "init", "(TBI) key <key>", "stash <filename>",
           "unstash <id> [filename]", "(TBI) delete <id>", "tag <id> <tag> ...", "(TBI) untag <id> <tag>",
           "search <tag> ...", "files", "tags");
}

/**
 * Joins a vector of strings together by a delimiter.
 *
 * @param strings The strings to join.
 * @param delim The delimiter to join the strings by.
 * @return
 */
std::string join(const std::vector<std::string> &strings, const std::string &delim) {
    std::string result;
    for(int i = 0; i < strings.size(); i++) {
        result += strings[i];
        if(i + 1 < strings.size()) // not at end, add delim
            result += delim;
    }
    return result;
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

        // set escaped status if backslash
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
            continue;
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
 * Prints a list of blobs and their properties to stdout.
 *
 * @param blobs A vector of blobs to print.
 */
void printBlobs(const std::vector<Tfc::BlobRecord*> &blobs) {

    // determine the size of the name field
    unsigned long nameSize = 10; // default to 10 chars
    for(Tfc::BlobRecord* record : blobs)
        if(record->getName().size() > 10)
            nameSize = record->getName().size();

    // build template strings for printing
    std::string headerTemplate = std::string("%-10s\t%-" + std::to_string(nameSize) + "s\t%-16s\t%-10s\n");
    std::string nameTemplate = std::string("%-" + std::to_string(nameSize) + "s\t");

    // print info for the resultant blobs
    printf(headerTemplate.c_str(), "ID", "Name", "Hash", "Tags");
    printf(headerTemplate.c_str(), "----------", "----------", "----------", "----------");
    for (Tfc::BlobRecord* record : blobs) {

        // print nonce
        printf("%-10d\t", record->getNonce());

        // print name
        printf(nameTemplate.c_str(), record->getName().c_str());

        // print hash as hex
        printf("%llx\t", static_cast<unsigned long long>(record->getHash()));

        // build vector of tag names
        std::vector<std::string> tagNames;
        for(auto tag : record->getTags())
            tagNames.push_back(tag->getName());

        // join and print tag names
        printf("%s\n", join(tagNames, ", ").c_str());

    }

}

/**
 * Splits a string into a vector of strings by a delimiter.
 *
 * @param string The string to split.
 * @param delim The delimiter to split by.
 * @return A vector of strings split from the complete string.
 */
std::vector<std::string> split(const std::string &string, char delim) {
    std::stringstream stream(string);
    std::vector<std::string> tokens;
    std::string token;
    while(std::getline(stream, token, delim))
        tokens.push_back(token);
    return tokens;
}

/**
 * Reads a file from the filesystem and writes it to the container.
 *
 * @param file The TFC file object
 * @param filename The name of the file to display
 * @param path The path of the file to stash
 * @return The ID that was assigned to the stashed file.
 */
uint32_t stash(Tfc::TfcFile* file, const std::string &filename, const std::string &path) {

    // read in file
    std::ifstream stream;
    stream.open(path, std::ios::binary | std::ios::in | std::ios::ate);
    if(stream.fail())
        throw Tfc::TfcFileException("Failed to open file " + path + " for reading");

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
    uint32_t nonce = file->addBlob(filename, data, static_cast<uint64_t>(size));
    file->mode(Tfc::TfcFileMode::CLOSED);

    // clean up
    delete [] data;

    return nonce;
}

/**
 * Returns a string with a colored status indicator based on whether a command succeeded or failed.
 */
std::string status(bool success) {
    std::string statusStr = "[" + std::string(Terminal::Decorations::BOLD);
    if(success)
        statusStr += std::string(Terminal::Colors::GREEN) + std::string(Terminal::Symbols::CHECKMARK);
    else
        statusStr += std::string(Terminal::Colors::RED) + std::string(Terminal::Symbols::CROSSMARK);
    statusStr += std::string(Terminal::Decorations::RESET) + "]";
    return statusStr;
}

/**
 * Reads a file from the container and writes it to the filesystem.
 *
 * @param file The TFC file object
 * @param id The ID of the file to unstash
 * @param filename The name the file should be given when it is written to the filesystem. Original name if not
 *                 specified.
 */
Tfc::BlobRecord* unstash(Tfc::TfcFile* file, uint32_t id, const std::string &filename) {
    Tfc::BlobRecord* record;

    // read the blob from the container
    file->mode(Tfc::TfcFileMode::READ);
    Tfc::TfcFileBlob* blob = file->readBlob(id);
    file->mode(Tfc::TfcFileMode::CLOSED);
    if(blob == nullptr)
        throw Tfc::TfcFileException("No file with that ID exists");
    record = blob->record;

    // determine the filename
    std::string blobFilename;
    if(filename.empty())
        blobFilename = blob->record->getName();
    else
        blobFilename = filename;

    // open the file for writing
    std::ofstream stream(blobFilename, std::ios::out | std::ios::binary);
    if(stream.fail())
        throw Tfc::TfcFileException("Failed to open file " + blobFilename + " for writing");
    stream.write(blob->data, blob->size);
    stream.close();

    // clean up
    delete [] blob->data;
    delete blob;

    return record;
}
