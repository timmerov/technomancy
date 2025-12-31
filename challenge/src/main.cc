/*
Copyright (C) 2012-2026 tim cotter. All rights reserved.
*/

/**
one billion lines challenge.
**/

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <errno.h>

#include <fstream>
#include <sstream>

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

    /** open file file and map it to memory. **/
    int fd = open(kMeasurementsFile, O_RDONLY);
    struct stat info;
    int result = fstat(fd, &info);
    (void) result;
    size_t length = info.st_size;
    auto map = (char *) mmap(NULL, length, PROT_READ, MAP_SHARED, fd, 0);

    /** get the first 20 characters. **/
    std::string line(map, 20);
    LOG(line);

    /** clean up. **/
    munmap(map, length);
    close(fd);

    LOG("Done!");

    return 0;
}
