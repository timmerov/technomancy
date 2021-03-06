/*
Copyright (C) 2012-2020 tim cotter. All rights reserved.
*/

/**
implementation of utilities and platform abstractions.
**/

#include <aggiornamento/aggiornamento.h>
#include <aggiornamento/log.h>

#include <algorithm>
#include <cctype>
#include <cstring>
#include <fstream>
#include <mutex>
#include <sstream>


agm::log::Lock agm::log::lock;
agm::log::Unlock agm::log::unlock;

namespace {
    class LogStreams {
    public:
        bool prefix_;
        std::ofstream file_;
        std::stringstream str_;
    };

    LogStreams *getLogStreams() noexcept {
        static LogStreams g_log_streams;
        return &g_log_streams;
    }

    std::mutex *getLogMutex() noexcept {
        static std::mutex g_mutex;
        return &g_mutex;
    }

    void logLineOfBytes(
        int index,
        const char *ptr,
        int count
    ) noexcept {
        std::string s;
        s.reserve(4+1+32*3+32);
        static const char hexdigits[] = "0123456789ABCDEF";
        auto ch1 = hexdigits[(index>>12)&0xf];
        auto ch2 = hexdigits[(index>>8)&0xf];
        auto ch3 = hexdigits[(index>>4)&0xf];
        auto ch4 = hexdigits[index&0xf];
        s += ch1;
        s += ch2;
        s += ch3;
        s += ch4;
        s += ' ';
        for (auto i = 0; i < count; ++i) {
            unsigned char x = ptr[i];
            auto ch5 = hexdigits[x>>4];
            auto ch6 = hexdigits[x&0xf];
            s += ch5;
            s += ch6;
            s += ' ';
        }
        for (auto i = 0; i < count; ++i) {
            char ch7 = ptr[i];
            unsigned char uch = ch7; // wtf?
            if (std::isprint(uch) == false) {
                ch7 = '.';
            }
            s += ch7;
        }
        LOG(s.c_str());
    }
}

void agm::log::init(
  const char *filename,
  bool prefix
) noexcept {
    auto ls = getLogStreams();
    ls->prefix_ = prefix;
    if (ls->file_.is_open() == false) {
        ls->file_.open(filename, std::ios::out | std::ios::trunc);
    }
}

void agm::log::exit() noexcept {
    auto ls = getLogStreams();
    ls->file_.close();
}

std::ostream *agm::log::getStream() noexcept {
    auto ls = getLogStreams();
    return &ls->str_;
}

std::string agm::log::getPrefix(
    const std::string& file,
    int line,
    const std::string& func
) noexcept {
  std::string s;
  auto ls = getLogStreams();
  if (ls->prefix_) {
    s += file + ":" + std::to_string(line) + ":" + func + ": ";
  }
  return s;
}

agm::log::AsHex::AsHex(
    int hex
) noexcept :
    value_(hex) {
}

void agm::log::bytes(
    const void *vp,
    int size
) noexcept {
    auto ptr = (const char *) vp;
    for (auto i = 0; size > 0; i += 24) {
        auto n = std::min(size, 24);
        logLineOfBytes(i, ptr, n);
        size -= n;
        ptr  += n;
    }
}

std::string agm::basename(
	const std::string& path
) noexcept {
	int where = path.find_last_of("/\\");
	auto file = path.substr(where+1);
	return file;
}

std::ostream & operator<<(
    std::ostream &s,
    const agm::log::Lock &lock
) noexcept {
    (void) lock;
    auto m = getLogMutex();
    m->lock();
    return s;
}

std::ostream & operator<<(
    std::ostream &s,
    const agm::log::Unlock &unlock
) noexcept {
    (void) unlock;
    auto ls = getLogStreams();
    // write to the file.
    ls->file_ << ls->str_.str();
    // write to the console.
#if defined(AGM_WINDOWS)
    OutputDebugStringA(ls->str_.str().c_str());
#endif
    std::cout << ls->str_.str();
    // clear the string.
    ls->str_.str(std::string());
    auto m = getLogMutex();
    m->unlock();
    return s;
}

std::ostream & operator<<(
    std::ostream &s,
    const agm::log::AsHex &hex
) noexcept {
    s << "0x" << std::hex << hex.value_ << std::dec;
    return s;
}
