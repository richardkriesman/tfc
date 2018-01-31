#include <iostream>
#include "libtfc/TfcFile.h"

int main(int argc, char** argv) {

    // read in test file
    std::ifstream stream;
    stream.open("/home/richard/Desktop/tfc-test.jpg", std::ios::binary | std::ios::in | std::ios::ate);
    std::streamsize size = stream.tellg();
    stream.seekg(0, stream.beg);
    auto* data = new char[size];
    stream.read(data, size);
    stream.close();

    // write empty file
    TfcFile file = TfcFile("container.tfc");
    file.mode(TfcFileMode::CREATE);
    file.init();
    file.mode(TfcFileMode::READ);
    file.mode(TfcFileMode::EDIT);
    file.addBlob(data, size);
    file.mode(TfcFileMode::CLOSED);

    return 0;

}

/**
 * Prints help text
 */
void help() {
    printf("Usage: tfc <command>\n"
                   "\t--version\tprints the version\n"
                   "\t--help\t\tprints this help page\n\n"
                   "Commands:\n"
                   "\tstash <filename>\tcopies a file into the container\n"
                   "\tunstash <id>\t\tcopies a file out of the container\n"
                   "\tdelete <id>\t\t\tdeletes a file from the container\n"
                   "\ttag <id> <tag>\t\tadds a tag to a file\n"
                   "\tuntag <id> <tag>\tremoves a tag from a file\n"
                   "\tsearch <tag> ...\tsearches for files matching all of the tags\n");
}