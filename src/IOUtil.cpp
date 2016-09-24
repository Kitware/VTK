#include "IOUtil.h"

#include <cstring>

IOUtil* IOUtil::instance = NULL;

IOUtil::IOUtil()
{
    isBigEndian = checkIfBigEndian();
}

int IOUtil::readShortInteger(int pos, ifstream &in) {
    in.seekg(pos, in.beg);
    char buffer[2];
    in.read(buffer, sizeof(buffer));

    if (!isBigEndian) {
        swap(buffer, buffer + 1);
    }

    short num;
    memcpy(&num, buffer, 2);
    return num;
}

int IOUtil::readLongInteger(int pos, ifstream &in) {
    in.seekg(pos, in.beg);
    char buffer[4];
    in.read(buffer, sizeof(buffer));

    if (!isBigEndian) {
        swap(buffer, buffer + 3);
        swap(buffer + 1, buffer + 2);
    }

    int num;
    memcpy(&num, buffer, 4);
    return num;
}

float IOUtil::readFloat(ifstream &in) {
    char buffer[4];
    in.read(buffer, sizeof(buffer));

    if (!isBigEndian) {
        swap(buffer, buffer + 3);
        swap(buffer + 1, buffer + 2);
    }

    float num;
    memcpy(&num, buffer, 4);
    return num;
}

char IOUtil::readChar(ifstream &in) {
    char buffer;
    in.read(&buffer, sizeof(buffer));
    return buffer;
}

void IOUtil::swap(char *a, char *b) {
    char temp = *a;
    *a = *b;
    *b = temp;
}

int IOUtil::getFileSize(ifstream &in) {
    in.seekg(0, in.end);
    return in.tellg();
}
