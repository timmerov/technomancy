/*
Copyright (C) 2012-2020 tim cotter. All rights reserved.
*/

/**
log utilities and platform wrappers
that should be part of the standard libraries
but aren't.

Note: if this file is very slow to open in codeblocks...
disable symbols browser.
[Menu] Settings -> Editor... -> [Icon] Code Completion ->
    [Tab] Symbols Browser -> [Checkbox] Disable symbols browser
**/

#pragma once

#include <aggiornamento/aggiornamento.h>

// c++ stl
#include <cstdarg>
#include <iostream>
#include <string>


// handy macro for logging
#define LOG(...) *agm::log::getStream()<<agm::log::lock<< \
    agm::log::getPrefix(agm::basename(__FILE__),__LINE__,__FUNCTION__) \
    <<__VA_ARGS__<<std::endl \
    <<agm::log::unlock

// handy macro for logging once
#define LOG_ONCE(...) { \
    static bool logged_once = false; \
    if (logged_once == false) { \
        logged_once = true; \
        LOG("(once) "<<__VA_ARGS__); \
    } \
}

// handy macro for logging way too much
#if !defined(LOG_VERBOSE)
#define LOG_VERBOSE(...)
#endif


namespace agm {
    // standardize logging.
    namespace log {
        void init(const char *filename, bool prefix = true) noexcept;
        void exit() noexcept;
        std::ostream *getStream() noexcept;
        std::string getPrefix(
            const std::string& file,
            int line,
            const std::string& func
        ) noexcept;

        // clever use of a lock to serialize logging.
        class Lock { public: };
        class Unlock { public: };
        extern Lock lock;
        extern Unlock unlock;

        // log int's in hexadecimal format
        class AsHex {
        public:
            AsHex(int hex) noexcept;
            int value_;
        };

        // log bytes in canonical form
        void bytes(const void *bytes, int size) noexcept;
    }

    std::string basename(const std::string& path) noexcept;
}

/*
clever use of a lock to serialize logging.
we basically insert a lock and unlock into the stream.
*/
std::ostream & operator<<(std::ostream &s, const agm::log::Lock &lock) noexcept;
std::ostream & operator<<(std::ostream &s, const agm::log::Unlock &unlock) noexcept;

/*
log int's in hexadecimal format
*/
std::ostream & operator<<(std::ostream &s, const agm::log::AsHex &x) noexcept;
