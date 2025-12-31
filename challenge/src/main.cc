/*
Copyright (C) 2012-2026 tim cotter. All rights reserved.
*/

/**
one billion lines challenge.

to do:
try memory mapped files: mmap.
**/

#include <fstream>
#include <sstream>
#include <stdio.h>

#include <aggiornamento/aggiornamento.h>
#include <aggiornamento/log.h>


int main(
    int argc, char *argv[]
) noexcept {
    (void) argc;
    (void) argv;

    constexpr auto kMeasurementsFile = "/media/ramdisk/measurements.txt";
    //constexpr auto kMeasurementsFile = "/home/timmer/Documents/code/1brc/measurements.txt";
    //constexpr auto kMeasurementsFile = "/home/timmer/Documents/code/1brc/measurements-1b.txt";

    agm::log::init(AGM_TARGET_NAME ".log");

    std::ifstream file(kMeasurementsFile);
    if (file.is_open() == false) {
        LOG("Failed to open file: "<<kMeasurementsFile);
        return -1;
    }

    /** 8 seconds **/
    file.seekg(0, std::ios::end);
    std::size_t sz = file.tellg();
    auto buffer = new(std::nothrow) char[sz+1];
    file.seekg(0);
    file.read(buffer, sz);
    buffer[sz] = 0;

    delete[] buffer;
    file.close();

    LOG("Done!");

    return 0;
}
