#include <iostream>
#include "libtfc/TfcFile.h"

const std::string VERSION = "0.0.1";

void help();
uint32_t stash(Tfc::TfcFile* file, const std::string &filename);
void unstash(Tfc::TfcFile* file, uint32_t id, const std::string &filename);

int main(int argc, char** argv) {

    // check for proper number of arguments
    if(argc < 3) {
        help();
        return 1;
    }

    // try to open a file
    Tfc::TfcFile* file = nullptr;
    try {
        std::string filename(argv[1]);
        std::string extension(".tfc");
        file = new Tfc::TfcFile(filename + extension);
    } catch (Tfc::TfcFileException &ex) {
        std::cerr << "tfc: " << ex.what() << "\n";
        return 1;
    }

    // handle commands
    std::string command = std::string(argv[2]);
    try {

        // help command
        if (command == "help") {
            help();

        // version command
        } else if(command == "version") {
            std::cout << "Tagged File Containers (TFC) v" << VERSION << "\n";

        // init command
        } else if (command == "init" && argc >= 3) {
            file->mode(Tfc::TfcFileMode::CREATE);
            file->init();
            file->mode(Tfc::TfcFileMode::CLOSED);
            std::cout << "Created container file at " << argv[1] << "\n";

        // list command
        } else if(command == "list") {
            file->mode(Tfc::TfcFileMode::READ);
            Tfc::JumpTableList* list = file->listBlobs();
            for(uint32_t i = 0; i < list->count; i++) {
                printf("%10d\t", list->rows[i]->nonce); // print nonce

                // print hash
                for (char j : list->rows[i]->hash) {
                    unsigned char byte = 0x00;
                    byte |= j;

                    printf("%02X ", (unsigned int)(byte & 0xFF));
                }

                printf("\n");
            }

        // stash command
        } else if (command == "stash" && argc >= 4) {
            uint32_t nonce = stash(file, argv[3]);
            std::cout << "Stashed " << argv[3] << " with ID " << nonce << "\n";

        // unstash command
        } else if (command== "unstash" && argc >= 5) {
            int32_t nonce = std::stoi(argv[3]);
            if(nonce < 0)
                throw Tfc::TfcFileException("File IDs cannot be negative");
            unstash(file, static_cast<uint32_t>(nonce), argv[4]);
            std::cout << "Unstashed " << nonce << " into " << argv[4];

        // unknown command
        } else {
            help();
            return 1;
        }

    } catch (Tfc::TfcFileException &ex) {
        std::cerr << "tfc: " << ex.what() << "\n";
        return 1;
    } catch (std::exception &ex) {
        std::cerr << "tfc: " << ex.what() << "\n";
        return 1;
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
                   "Usage: tfc <filename> <command>\n"
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
    "version", "help", "init", "stash <filename>", "unstash <id> <filename>", "delete <id>",
           "tag <id> <tag>", "untag <id> <tag>", "search <tag> ...", "list");
}