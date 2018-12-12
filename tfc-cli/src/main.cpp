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
#include <iostream>
#include <vector>
#include <sstream>
#include <unistd.h>
#include <algorithm>
#include <cmath>
#include <csignal>
#include <mutex>
#include <tasker/tasker.h>
#include <tfc/container.h>
#include "terminal.h"
#include "license.h"

#define VERSION "0.2.1"

/**
 * Command result types
 */
enum ResultType {
    SUCCESS,
    WARNING,
    FAILURE,
    OUTPUT,
    INFO
};

/**
 * Forward declarations
 */
void about();
void await(Tasker::Task* task, const std::string &message);
void help();
std::string join(const std::vector<std::string> &strings, const std::string &delim);
int license();
int license(std::string name);
std::vector<std::string> parseInput(const std::string &input);
void printBlobs(const std::vector<Tfc::FileRecord*> &blobs);
std::vector<std::string> split(const std::string &string, char delim);
uint32_t stash(Tfc::Container* file, const std::string &filename, const std::string &path);
std::string status(ResultType resultType);
Tfc::ReadableFile* unstash(Tfc::Container* container, uint32_t id, const std::string &filename = "");

/**
 * Global variables
 */
std::mutex lock; // a lock on global vars
bool shouldStop = false; // whether the loop should be stopped
bool idle = false; // whether the loop is idle

/**
 * Handler for system stop signals
 */
void handleSignal(int sigNum) {
    lock.lock();
    if (idle) { // input is idle, stop immediately
        std::cout << "\n";
        exit(0);
    } else {
        std::cout << Terminal::Cursor::HOME << status(ResultType::INFO)
                  << "Stopping..." << Terminal::Cursor::up(1) << Terminal::Cursor::END;
        shouldStop = true;
    }
    lock.unlock();
}

/**
 * The main function. Parses and runs commands.
 */
int main(int argc, char** argv) {
    bool isInteractive = false; // whether the client is operating in interactive mode

    // trap signals to allow for safe shutdown
    std::signal(SIGINT, handleSignal);
    std::signal(SIGTERM, handleSignal);
    std::signal(SIGHUP, handleSignal);

    // check for proper number of arguments
    if(argc < 2) {
        help();
        return 1;
    } else if (argc == 2) { // only filename was provided, run in interactive mode
        isInteractive = true;
    }
    std::string filename = argv[1]; // filename, may also be --help or --version

    // check for version or help flags
    if(filename == "--help") { // help flag
        help();
        return 0;
    } else if(filename == "--version") { // version flag
        std::cout << "Tagged File Containers (TFC) v" << VERSION << "\n";
        return 0;
    } else if(filename == "--about") {
        about();
        return 0;
    } else if(filename == "--license") {
        if (argc > 2) {
            return license(argv[2]);
        } else {
            return license();
        }
    }

    // try to open a file
    Tfc::Container* file = nullptr;
    try {
        file = new Tfc::Container(filename);
        if(file->doesExist())
            file->mode(Tfc::OperationMode::READ);
    } catch (Tfc::Exception &ex) {
        std::cerr << "tfc: " << ex.what() << "\n";
        return 1;
    }

    // create an event loop
    Tasker::Loop loop;
    loop.start();

    // if non-interactive mode, build a list of commands to be parsed
    std::vector<std::string> commands;
    if(!isInteractive) {
        std::string command; // current command under construction

        // parse each arg
        for(int i = 2; i < argc; i++) {
            std::string arg(argv[i]);

            // determine if this is a new command (starts with --)
            bool isNewCommand = arg.find("--") == 0;

            // handle new commands
            if(isNewCommand) {
                if(!command.empty()) // not first command, add current one to the command list
                    commands.push_back(command);
                command = arg.substr(2);
                continue;
            }

            // add arg to command string
            if(!command.empty()) // valid command has been started
                command += " " + arg;

        }

        // add last command string to command vector
        if(!command.empty()) // valid command has been started
            commands.push_back(command);

        // invalid command structure, show help
        if(commands.empty()) {
            std::cerr << "tfc: Invalid command syntax\n";
            return 1;
        }
    }

    // print welcome message if file does not exist
    if (!file->doesExist()) {
        std::cout << Terminal::Symbols::TIP << " " << Terminal::Decorations::BOLD << Terminal::Foreground::GREEN
                  << "Welcome to TFC! " << Terminal::Decorations::RESET << "Type `init` to create a new container file "
                  << "at " << filename << ".\n";
    }

    // print encryption warning if file is not encrypted
    if (!file->isEncrypted() && file->doesExist()) {
        std::cout << status(ResultType::WARNING) << "This container is not encrypted. Files can be unstashed by anyone!\n";
    }

    // handle commands
    std::string input;
    int currentCommand = 0; // current command index when running in non-interactive mode
    while ((input != "exit" && isInteractive) || currentCommand < commands.size()) {

        // check if the loop is being interrupted
        lock.lock();
        if (shouldStop) {
            lock.unlock();
            break;
        }
        lock.unlock();

        // print prompt
        if (file->isUnlocked() && file->doesExist()) {
            std::cout << Terminal::Symbols::UNLOCKED << Terminal::Foreground::GREEN << " ";
        } else {
            if (!file->doesExist()) {
                std::cout << Terminal::Symbols::QUESTION << "  ";
            } else if (!file->isUnlocked()) {
                std::cout << Terminal::Symbols::LOCKED;
            }
            std::cout << Terminal::Foreground::GREY;
        }
        std::cout << "tfc> " << Terminal::Decorations::RESET;

        // get input
        if(isInteractive) { // interactive mode, prompt for input
            lock.lock();
            idle = true;
            lock.unlock();
            std::getline(std::cin, input);
            lock.lock();
            idle = false;
            lock.unlock();
        } else { // non-interactive mode, get input commands vector
            input = commands[currentCommand++];
            std::cout << input << "\n";
        }

        // parse input
        std::vector<std::string> args = parseInput(input);

        // process input
        try {

            // help command
            if (args[0] == "help") {
                help();
                continue;
            }

            // about command
            if (args[0] == "about") {
                about();
                continue;
            }

            // license command
            if (args[0] == "license") {
                if (args.size() > 1) {
                    license(args[1]);
                } else {
                    license();
                }
                continue;
            }

            // clear screen command
            if (args[0] == "clear") {
                std::cout << Terminal::Screen::CLEAR << status(ResultType::SUCCESS) << "Cleared screen\n";
                continue;
            }

            // init command
            if (args[0] == "init") {
                file->mode(Tfc::OperationMode::CREATE);
                file->init();
                file->mode(Tfc::OperationMode::READ);
                std::cout << status(ResultType::SUCCESS) << "Created container file at " << filename << "\n";
                continue;
            }

            // list files command
            if(args[0] == "files") {
                file->mode(Tfc::OperationMode::READ);
                printBlobs(file->listBlobs());
                continue;
            }

            // list tags command
            if(args[0] == "tags") {
                file->mode(Tfc::OperationMode::READ);
                std::vector<Tfc::TagRecord*> tags = file->listTags();

                // determine the longest tag name
                unsigned long nameLength = 10; // default to 10 chars
                for(Tfc::TagRecord* tag : tags) {
                    if(tag->getName().length() > nameLength)
                        nameLength = tag->getName().length();
                }

                // build string templates
                std::string headerTemplate = status(ResultType::OUTPUT) +
                        std::string("%-" + std::to_string(nameLength) + "s\t%-10s\n");
                std::string rowTemplate = status(ResultType::OUTPUT) +
                        std::string("%-" + std::to_string(nameLength) + "s\t%-10lu\n");

                // print headers
                printf(headerTemplate.c_str(), "Name", "File Count");
                printf(headerTemplate.c_str(), "----------", "----------");

                // print tags
                for(Tfc::TagRecord* tag : file->listTags())
                    printf(rowTemplate.c_str(), tag->getName().c_str(), tag->getBlobs()->size());

                continue;
            }

            // stash command
            if (args[0] == "stash" && args.size() == 2) {

                // determine the name of the file
                std::vector<std::string> path = split(args[1], '/');
                std::string name = path[path.size() - 1];

                // stash the file
                auto* stashTask = new Tasker::Task([&file, &name, &args](Tasker::TaskHandle* handle) -> void* {
                    auto* nonce = new uint32_t();
                    *nonce = stash(file, name, args[1]);

                    return static_cast<void*>(nonce);
                });

                // show animation while the file is stashed
                loop.run(stashTask);
                await(stashTask, "Stashing " + name);
                if (stashTask->getState() == Tasker::TaskState::FAILED)
                    std::rethrow_exception(stashTask->getException());

                // extract the nonce
                auto* nonce = static_cast<uint32_t*>(stashTask->getResult());

                std::cout << status(ResultType::SUCCESS) << "Stashed " << name << " with ID " << *nonce << "\n";
                delete nonce;
                delete stashTask;

                continue;
            }

            // unstash command
            if (args[0] == "unstash" && (args.size() == 2 || args.size() == 3)) {
                int32_t nonce = std::stoi(args[1]);
                if (nonce < 0)
                    throw Tfc::Exception("Container IDs cannot be negative");

                // unstash the file
                Tasker::Task* task = new Tasker::Task([&file, nonce, &args](Tasker::TaskHandle* handle) -> void* {
                    auto* name = new std::string();

                    Tfc::ReadableFile* result;
                    if(args.size() == 2) { // use original filename
                        result = unstash(file, static_cast<uint32_t>(nonce));
                        *name = result->getFilename();
                    } else { // use explicit filename
                        result = unstash(file, static_cast<uint32_t>(nonce), args[2]);
                        *name = args[2];
                    }
                    delete result;

                    return static_cast<void*>(name);
                });

                // wait for the task to complete
                loop.run(task);
                await(task, "Unstashing file");
                if (task->getState() == Tasker::TaskState::FAILED)
                    std::rethrow_exception(task->getException());

                // extract the name
                auto* name = static_cast<std::string*>(task->getResult());

                // output success message
                std::cout << status(ResultType::SUCCESS) << "Unstashed " << nonce << " into " << *name << "\n";
                delete name;
                delete task;

                continue;
            }

            // delete command
            if (args[0] == "delete" && args.size() == 2) {
                int32_t nonce = std::stoi(args[1]);
                if (nonce < 0)
                    throw Tfc::Exception("Container IDs cannot be negative");

                // set file mode to EDIT
                file->mode(Tfc::OperationMode::EDIT);

                // delete the blob
                Tasker::Task* task = new Tasker::Task([&file, nonce](Tasker::TaskHandle* handle) -> void* {
                    file->deleteBlob(static_cast<uint32_t>(nonce));

                    return nullptr;
                });

                // wait for the task to complete
                loop.run(task);
                await(task, "Deleting file");
                if (task->getState() == Tasker::TaskState::FAILED)
                    std::rethrow_exception(task->getException());

                // output success message
                std::cout << status(ResultType::SUCCESS) << "Deleted " << nonce << "\n";
                delete task;

                continue;
            }

            // tag command
            if (args[0] == "tag" && args.size() >= 3) {
                int32_t nonce = std::stoi(args[1]);
                if (nonce < 0)
                    throw Tfc::Exception("Container IDs cannot be negative");

                // attach the tags
                file->mode(Tfc::OperationMode::READ);
                file->mode(Tfc::OperationMode::EDIT);
                for(int i = 2; i < args.size(); i++) {
                    file->attachTag(static_cast<uint32_t>(nonce), args[i]);
                    std::cout << status(ResultType::SUCCESS) << "Tagged " << nonce << " as " << args[i] << "\n";
                }

                continue;
            }

            // search command
            if (args[0] == "search" && args.size() > 1) {

                // build a set of tags
                std::vector<std::string> tags;
                for(int i = 1; i < args.size(); i++)
                    tags.push_back(args[i]);

                // get a set of intersecting blobs
                std::vector<Tfc::FileRecord*> intersection;
                file->mode(Tfc::OperationMode::READ);
                intersection = file->intersection(tags);

                // print intersecting blobs
                printBlobs(intersection);

                continue;

            }

            // unknown command
            if(args[0] != "exit")
                std::cerr << status(ResultType::FAILURE) << "Invalid command. Type \"help\" for a list of commands.\n";

        } catch (Tfc::Exception &ex) {
            std::cerr << status(ResultType::FAILURE) << ex.what() << "\n";
        } catch (std::exception &ex) {
            std::cerr << status(ResultType::FAILURE) << ex.what() << "\n";
        }

    }

    // close the file
    file->mode(Tfc::OperationMode::CLOSED);

    // stop the event loop
    loop.stop();
    loop.wait();

    // clean up
    delete file;

    return 0;

}

/**
 * Prints the about page and copyright information.
 */
void about() {
    std::cout << Terminal::Decorations::BOLD <<  "Tagged File Containers (TFC)\n\n"
              << Terminal::Decorations::RESET
              << "Copyright © Richard Kriesman 2018.\n"
                 "https://richardkriesman.com\n\n"
                 "This program comes with ABSOLUTELY NO WARRANTY.\n"
                 "This is free software, and you are welcome to redistribute it\n"
                 "under certain conditions. Type `about license` for details.\n\n"
                 "tfc-cli was made with help from the following third-party libraries:\n"
                 "\txxhash\n"
                 "\tPOCO\n"
                 "You can type `license <name>` to view the license of any library.\n";
}

/**
 * Blocks the current thread, showing a spinner and message to the user while a Task runs.
 *
 * @param task The task to await.
 * @param message A message to display to the user explaining the operation.
 */
void await(Tasker::Task* task, const std::string &message) {
    const std::string states[] = { Terminal::Symbols::CLOCK_12, Terminal::Symbols::CLOCK_1,
        Terminal::Symbols::CLOCK_2, Terminal::Symbols::CLOCK_3, Terminal::Symbols::CLOCK_4,
        Terminal::Symbols::CLOCK_5, Terminal::Symbols::CLOCK_6, Terminal::Symbols::CLOCK_7,
        Terminal::Symbols::CLOCK_8, Terminal::Symbols::CLOCK_9, Terminal::Symbols::CLOCK_10,
        Terminal::Symbols::CLOCK_11 }; // animation states
    int i = 0; // current animation state

    // print message
    std::cout << states[i] << " " << message;

    // animate spinner until task is done
    Tasker::TaskState state = task->getState();
    while(state != Tasker::TaskState::COMPLETED && state != Tasker::TaskState::FAILED) {

        // update animation state
        std::cout << Terminal::Cursor::HOME << states[i] << Terminal::Cursor::END << std::flush;

        // move to next animation state
        if(i < 11)
            i++;
        else
            i = 0;

        // wait a little bit before trying again
        usleep(75000);
        state = task->getState();

    }

    // delete the "in progress line"
    std::cout << Terminal::Cursor::HOME << Terminal::Cursor::ERASE_EOL << std::flush;
}

/**
 * Prints help text
 */
void help() {
    printf("Tagged Container Containers\n\n"
                   "Usage: tfc <filename> [commands]...\n"
                   "\t%-25s\tprints copyright information\n"
                   "\t%-25s\tprints this help page\n"
                   "\t%-25s\tprints the license\n"
                   "\t%-25s\tprints the version\n\n"
                   "Commands:\n"
                   "\t%-25s\tprints this help page\n"
                   "\t%-25s\tprints copyright information\n"
                   "\t%-25s\tprints the license\n"
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
                   "\t%-25s\tlists all tags by their name\n\n"
                   "Interactive Mode:\n"
                   "\tYou can start tfc in interactive mode by omitting commands in the \n"
                   "\tcommand line. Only the filename should be specified. Interactive mode \n"
                   "\tis useful when using tfc as a human operator because it will shorten \n"
                   "\tcommand invocations and store the container's encryption key in memory \n"
                   "\tduring the session.\n\n"
                   "Non-interactive Mode:\n"
                   "\tYou can start tfc in non-interactive mode by passing command-line \n"
                   "\targuments to tfc. This can be useful when using tfc with scripting. \n"
                   "\tCommands can be run in non-interactive mode by prefixing the command \n"
                   "\twith --. For example, `--stash cute-cat.png`.\n",
           "--about", "--help", "--license", "--version", "help", "about", "license", "clear", "init",
           "(TBI) key <key>", "stash <filename>", "unstash <id> [filename]", "delete <id>", "tag <id> <tag> ...",
           "(TBI) untag <id> <tag>", "search <tag> ...", "files", "tags");
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
 * Prints the license.
 */
int license() {
    std::cout << LICENSE;
    return 0;
}

/**
 * Prints the license of a third-party library.
 *
 * @param name The name of the library
 */
int license(std::string name) {
    std::transform(name.begin(), name.end(), name.begin(), ::tolower); // convert the name to lowercase for comparison

    // check name
    if (name == "xxhash") {
        std::cout << XXHASH_LICENSE;
        return 0;
    } else if (name == "poco") {
        std::cout << POCO_LICENSE;
        return 0;
    } else {
        std::cout << Terminal::Symbols::CROSSMARK << " Invalid name\n";
        return 1;
    }
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
void printBlobs(const std::vector<Tfc::FileRecord*> &blobs) {
    const int ID_LEN       = 10; // length of the id column
    const int HASH_LEN     = 16; // length of the hash column
    const int TAGS_LEN     = 10; // length of the tag column header
    const int MAX_LINE_LEN = 80; // maximum length of a line

    // build template strings for printing
    std::string headerTemplate = std::string("%-" + std::to_string(ID_LEN) + "s  %-" + std::to_string(HASH_LEN)
                                             + "s  %-" + std::to_string(TAGS_LEN) + "s\n");

    // print info for the resultant blobs
    unsigned long tagColStart = (ID_LEN + 2) + (HASH_LEN + 2); // start pos of the tag column
    printf(std::string(status(ResultType::OUTPUT) + headerTemplate).c_str(), "ID", "Hash", "Tags");
    printf(std::string(status(ResultType::OUTPUT) + headerTemplate).c_str(), "----------", "----------", "----------");
    for (Tfc::FileRecord* record : blobs) {

        // print nonce
        printf("%s%-10d  ", status(ResultType::OUTPUT).c_str(), record->getNonce());

        // print hash as hex
        printf("%llx  ", static_cast<unsigned long long>(record->getHash()));

        // build vector of tag names
        std::vector<std::string> tagNames;
        for(auto tag : *record->getTags())
            tagNames.push_back(tag->getName());

        // sort the tag names
        std::sort(tagNames.begin(), tagNames.end());

        // join and print tag names with proper line breaking
        unsigned long lineLength = tagColStart;
        for(unsigned long i = 0; i < tagNames.size(); i++) {
            auto tag = tagNames[i];

            // build the line to be printed
            std::string printTag = tag;
            if(i + 1 < tagNames.size())
                printTag += ", ";

            // if line exceeds max length, break to next line
            if(lineLength + printTag.length() > MAX_LINE_LEN) {
                printf("%c%s", '\n', status(ResultType::OUTPUT).c_str());
                lineLength = tagColStart;

                // print spacing up to start of tag column
                for(unsigned long j = 0; j < tagColStart; j++)
                    printf("%c", ' ');
            }

            // print tag
            printf("%s", printTag.c_str());
            lineLength += printTag.length();

        }
        printf("\n");

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
uint32_t stash(Tfc::Container* file, const std::string &filename, const std::string &path) {

    // read in file
    std::ifstream stream;
    stream.open(path, std::ios::binary | std::ios::in | std::ios::ate);
    if(stream.fail())
        throw Tfc::Exception("Failed to open file " + path + " for reading");

    // read stream size
    std::streamsize size = stream.tellg();
    stream.seekg(0, stream.beg);

    // read data
    auto* data = new char[size];
    stream.read(data, size);
    stream.close();

    // add data as blob
    file->mode(Tfc::OperationMode::READ);
    file->mode(Tfc::OperationMode::EDIT);
    uint32_t nonce = file->addBlob(filename, data, static_cast<uint64_t>(size));
    file->mode(Tfc::OperationMode::CLOSED);

    // clean up
    delete [] data;

    return nonce;
}

/**
 * Returns a string with a colored status indicator based on whether a command succeeded or failed.
 *
 * @param success Whether or not the command succeeded.
 * @return A formatted status prefix to be printed to the terminal with a message.
 */
std::string status(ResultType resultType) {
    std::string statusStr;
    switch (resultType) {
        case ResultType::SUCCESS:
            statusStr = Terminal::Symbols::CHECKMARK + std::string("  ");
            break;
        case ResultType::WARNING:
            statusStr += std::string("    ") + Terminal::Decorations::BOLD +
                         Terminal::Foreground::YELLOW + std::string("Warning: ") + Terminal::Decorations::RESET;
            break;
        case ResultType::FAILURE:
            statusStr += Terminal::Symbols::CROSSMARK + std::string(" ") + Terminal::Decorations::BOLD +
                         Terminal::Foreground::RED + std::string("Error: ") + Terminal::Decorations::RESET;
            break;
        case ResultType::OUTPUT:
            statusStr += Terminal::Foreground::GREEN + std::string(Terminal::Symbols::RIGHT_ARROW) +
                         std::string("  ") + Terminal::Decorations::RESET;
            break;
        case ResultType::INFO:
            statusStr += Terminal::Symbols::BELL + std::string(" ");
            break;
    }
    return statusStr;
}

/**
 * Reads a file from the container and writes it to the filesystem.
 *
 * @param container The TFC file object
 * @param id The ID of the file to unstash
 * @param filename The name the file should be given when it is written to the filesystem. Original name if not
 *                 specified.
 */
Tfc::ReadableFile* unstash(Tfc::Container* container, uint32_t id, const std::string &filename) {

    // get file to read from container
    Tfc::ReadableFile* file = container->readFile(id);

    // determine the filename
    std::string blobFilename;
    if(filename.empty()) {
        blobFilename = file->getFilename();
    } else {
        blobFilename = filename;
    }

    // open the file for writing
    std::ofstream stream(blobFilename.c_str(), std::ios::out | std::ios::binary);
    if(stream.fail()) {
        throw Tfc::Exception("Failed to open file " + blobFilename + " for writing");
    }

    // write bytes from the file to the output file
    uint64_t remainingBytes = file->getSize();
    while (remainingBytes > 0) {

        // determine number of bytes to write (up to 512 bytes)
        uint64_t bytesToWrite;
        if (remainingBytes >= Tfc::BLOCK_DATA_SIZE) {
            bytesToWrite = Tfc::BLOCK_DATA_SIZE;
        } else {
            bytesToWrite = remainingBytes;
        }

        // write bytes to file stream
        char* buf = file->readBlock();
        stream.write(buf, static_cast<std::streamsize>(bytesToWrite));
        remainingBytes -= bytesToWrite;
        delete buf;
    }
    stream.close();

    return file;
}
