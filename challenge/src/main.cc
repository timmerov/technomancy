/*
Copyright (C) 2012-2026 tim cotter. All rights reserved.
*/

/**
one billion lines challenge.

dataset optimizations:

divide the locations into 8 roughly equal size buckets.
1_000_000_000 / 8 = 125_000_000

table['a']=72_641_470 sum=0
table['b']=96_841_621 sum=72_641_470
table['c']=53_266_763 sum=169_483_091

table['d']=55_710_670 sum=222_749_854
table['e']=14_523_717 sum=278_460_524

table['f']=14_528_839 sum=292_984_241
table['g']=29_057_498 sum=307_513_080
table['h']=45_995_769 sum=336_570_578

table['i']=12_111_098 sum=382_566_347
table['j']=16_951_109 sum=394_677_445
table['k']=48_431_760 sum=411_628_554
table['l']=60_531_231 sum=460_060_314
table['m']=89_594_157 sum=520_591_545

table['n']=48_424_659 sum=610_185_702
table['o']=26_625_519 sum=658_610_361

table['p']=62_938_398 sum=685_235_880
table['q']=0          sum=748_174_278
table['r']=21_794_213 sum=748_174_278
table['s']=82_319_836 sum=769_968_491

table['t']=65_379_134 sum=852_288_327
table['u']= 4_840_654 sum=917_667_461

table['v']=26_636_511 sum=922_508_115
table['w']=21_794_372 sum=949_144_626
table['x']= 2_422_678 sum=970_938_998
table['y']=14_532_058 sum=973_361_676
table['z']= 7_260_837 sum=987_893_734
table[' ']= 4_845_429 sum=995_154_571

from the dataset:
min_line=7
max_line=32
max_precision=2
similar names (8): "Alexandria" and "Alexandra"
**/

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include <bit>
#include <iomanip>
#include <unordered_map>
#include <vector>

#include <aggiornamento/aggiornamento.h>
#include <aggiornamento/log.h>

constexpr int kLocationSize = 32;

class Temperature {
public:
    char location_[kLocationSize];
    double min_;
    double max_;
    double sum_;
    size_t count_;
};

class OneBillionRows {
public:
    OneBillionRows() = default;
    ~OneBillionRows() noexcept {
        cleanup();
    }

    static constexpr auto kMeasurementsFile = "/media/ramdisk/measurements.txt";
    //static constexpr auto kMeasurementsFile = "/home/timmer/Documents/code/1brc/measurements.txt";
    //static constexpr auto kMeasurementsFile = "/home/timmer/Documents/code/1brc/measurements-1b.txt";

    static constexpr int kMaxRecordsBits = 17;
    static constexpr int kMaxRecords = 1 << kMaxRecordsBits;
    static constexpr int kMaxRecordsMask = kMaxRecords - 1;

    int fd_ = -1;
    size_t length_ = 0;
    char *map_ = nullptr;

    /** this is large enough we don't want it on the stack. **/
    size_t table_[27];

    void run() noexcept {
        bool success = init();
        if (success == false) {
            return;
        }

        //analyzeDistribution();
        //analyzeTree();
        //unorderedTree();
        //unsortedArray();
        //hashTable();
        speedRun();
    }

    bool init() noexcept {
        /** open the file. **/
        fd_ = open(kMeasurementsFile, O_RDONLY);
        if (fd_ < 0) {
            LOG("Failed to open: \""<<kMeasurementsFile<<"\"");
            return false;
        }

        /** map the file to memory. **/
        struct stat info;
        int result = fstat(fd_, &info);
        (void) result;
        length_ = info.st_size;
        map_ = (char *) mmap(NULL, length_, PROT_READ, MAP_SHARED, fd_, 0);
        if (map_ == nullptr) {
            return false;
        }

        //length_ /= 30;

        return true;
    }

    void cleanup() noexcept {
        if (map_) {
            munmap((void *) map_, length_);
            map_ = nullptr;
        }
        if (fd_ >= 0) {
            close(fd_);
            fd_ = -1;
        }
    }

    void speedRun() noexcept {
        const char *src = map_;
        const char *limit = src + length_;

        static constexpr int kTemperatureSize = 8;

        char location[kLocationSize];
        char temperature[kTemperatureSize];

        //char *loc_limit = &location[kLocationSize];
        //char *temp_limit = &temperature[kTemperatureSize];

        agm::int64 count = 0;
        while (src < limit) {
            auto dst64 = (agm::int64 *) location;
            dst64[0] = 0LL;
            dst64[1] = 0LL;
            dst64[2] = 0LL;
            dst64[3] = 0LL;
            char *dst = location;
            for(;;) {
                char ch0 = src[0];
                char ch1 = src[1];
                if (ch0 == ';') {
                    src += 1;
                    break;
                }
                if (ch1 == ';') {
                    src += 2;
                    dst[0] = ch0;
                    break;
                }
                src += 2;
                dst[0] = ch0;
                dst[1] = ch1;
                dst += 2;
            }

            dst64 = (agm::int64 *) temperature;
            dst64[0] = 0;
            dst = temperature;
            for(;;) {
                char ch0 = src[0];
                char ch1 = src[1];
                if (ch0 == 0x0A) {
                    src += 1;
                    break;
                }
                if (ch1 == 0x0A) {
                    src += 2;
                    dst[0] = ch0;
                    break;
                }
                src += 2;
                dst[0] = ch0;
                dst[1] = ch1;
                dst += 2;
            }

            ++count;
            if ((count % (100*1000*1000LL)) == 0) {
                for (int i = 0; i < kLocationSize; ++i) {
                    if (location[i] == 0) {
                        location[i] = '.';
                    }
                }
                for (int i = 0; i < kTemperatureSize; ++i) {
                    if (temperature[i] == 0) {
                        temperature[i] = '.';
                    }
                }
                std::string loc(location, kLocationSize);
                std::string temp(temperature, kTemperatureSize);
                LOG("row["<<count<<"]: loc=\""<<loc<<"\" temp=\""<<temp<<"\"");
            }
        }
    }

    void hashTable() noexcept {
        auto map = map_;
        auto limit = map + length_;

        auto records = new(std::nothrow) Temperature[kMaxRecords];
        for (int i = 0; i < kMaxRecords; ++i) {
            records[i].location_[0] = 0;
        }

        std::vector<int> where;
        where.reserve(kMaxRecords);

        int nrecords = 0;
        //size_t count = 0;
        while (map < limit) {
            char location[kLocationSize];
            map = copyLocation(map, location);
            double temp = popTemperature(map);
            map = findChar(map, 0x0A);
            ++map;

            /*LOG("location="<<location<<" temp="<<temp);
            ++count;
            if (count >= 1000LL) {
                break;
            }*/
            /*if ((count % (1000*1000LL)) == 0) {
                LOG("count="<<count<<" nrecords="<<nrecords);
            }*/

            auto hash = computeHash(location);

            for (int i = 0; i < kMaxRecords; ++i) {
                auto test = records + hash;

                /** create new record. **/
                if (test->location_[0] == 0) {
                    ++nrecords;
                    where.push_back(hash);
                    std::memcpy(test->location_, location, kLocationSize);
                    test->min_ = temp;
                    test->max_ = temp;
                    test->sum_ = temp;
                    test->count_ = 1;
                    break;
                }

                /** update old record. **/
                if (locationsDiffer(test->location_, location) == 0) {
                //if (std::memcmp(test->location_, location, kLocationSize) == 0) {
                    test->min_ = std::min(test->min_, temp);
                    test->max_ = std::max(test->max_, temp);
                    test->sum_ += temp;
                    test->count_++;
                    break;
                }

                hash = (hash + 1) & kMaxRecordsMask;
            }
        }

        int ncollisions = 0;
        for (auto i : where) {
            auto &rec = records[i];
            double avg = rec.sum_ / double(rec.count_);
            LOG("tree["<<i<<"]=\""<<rec.location_<<"\";"<<rec.min_<<";"<<rec.max_<<";"<<avg);

            int hash = computeHash(rec.location_);
            if (hash != i) {
                ++ncollisions;
            }
        }
        LOG("tree.size="<<nrecords);
        LOG("ncollisions="<<ncollisions);

        delete[] records;
    }

    agm::uint64 locationsDiffer(
        char *a8,
        char *b8
    ) noexcept {
        auto a64 = (agm::uint64 *) a8;
        auto b64 = (agm::uint64 *) b8;
        agm::uint64 x = a64[0] ^ b64[0];
        x |= a64[1] ^ b64[1];
        x |= a64[2] ^ b64[2];
        x |= a64[3] ^ b64[3];
        return x;
    }

    unsigned int computeHash(
        char *location
    ) noexcept {
        auto hash = * (agm::uint64 *) location;
        hash ^= hash >> kMaxRecordsBits;
        hash ^= hash >> 2*kMaxRecordsBits;
        hash ^= hash >> 3*kMaxRecordsBits;
        hash &= kMaxRecordsMask;
        return (unsigned int) hash;
    }

    char *copyLocation(
        char *src,
        char *dst
    ) noexcept {
        /**
        this mask zeroes chars after the delimiter.
        the fist char of the string is in the least significant byte.
        the string is reversed in the int64.
        **/
        static constexpr agm::uint64 kMaskByPosition[] = {
            0x0000000000000000,
            0x00000000000000FF,
            0x000000000000FFFF,
            0x0000000000FFFFFF,
            0x00000000FFFFFFFF,
            0x000000FFFFFFFFFF,
            0x0000FFFFFFFFFFFF,
            0x00FFFFFFFFFFFFFF,
            0xFFFFFFFFFFFFFFFF,
        };
        /** this mask zeroes int64s after the delimiter. **/
        static constexpr agm::uint64 kMaskCarry[] = {
            0x0000000000000000,
            0x0000000000000000,
            0x0000000000000000,
            0x0000000000000000,
            0x0000000000000000,
            0x0000000000000000,
            0x0000000000000000,
            0x0000000000000000,
            0xFFFFFFFFFFFFFFFF,
        };

        /** load the location string into 4 int64s. **/
        auto src64 = (agm::uint64 *) src;
        auto dst64 = (agm::uint64 *) dst;
        auto x0 = src64[0];
        auto x1 = src64[1];
        auto x2 = src64[2];
        auto x3 = src64[3];

        /** find the delimiter in each int64. **/
        int pos0 = findDelimiter(x0);
        int pos1 = findDelimiter(x1);
        int pos2 = findDelimiter(x2);
        int pos3 = findDelimiter(x3);

        /** load the carry masks. **/
        auto mask1 = kMaskCarry[pos0];
        auto mask2 = kMaskCarry[pos1];
        auto mask3 = kMaskCarry[pos2];

        /** zero positions after the delimiter. **/
        auto mask12 = mask1 & mask2;
        pos1 &= mask1;
        pos2 &= mask12;
        pos3 &= mask12 & mask3;

        /** zero trailing bytes in the int64s. **/
        x0 &= kMaskByPosition[pos0];
        x1 &= kMaskByPosition[pos1];
        x2 &= kMaskByPosition[pos2];
        x3 &= kMaskByPosition[pos3];

        /** write the int64s. **/
        dst64[0] = x0;
        dst64[1] = x1;
        dst64[2] = x2;
        dst64[3] = x3;

        /** advance the pointer past the location text and the delimiter. **/
        src += pos0 + pos1 + pos2 + pos3 + 1;

        return src;
    }

    /**
    value holds a string.
    the first character of the string is in the low order byte.
    **/
    int findDelimiter(
        agm::uint64 value
    ) noexcept {
        /** erase all semicolons. **/
        auto x = value ^ 0x3B3B3B3B3B3B3B3BL;

        /** find the byte offset to the first semicolon. **/
        int pos = byteOffsetToFirstZero(x);
        return pos;
    }

    /**
    value holds a sequence of bytes.
    some of which are zero.
    find the number of trailing non-zero bytes.
    **/
    int byteOffsetToFirstZero(
        agm::uint64 value
    ) noexcept {
        /**
        magic formula to flip zero bytes to 0x80.
        and non-zero bytes to something between 0x00 and 0x7f.
        **/
        value = (value - 0x0101010101010101L) & ~value;

        /**
        bytes that were originally zero are now 0x80.
        bytes that were anything else are now 0x00.
        **/
        value &= 0x8080808080808080L;

        /** builtin function returns number of trailing zero bits. **/
        int zerobits = std::countr_zero(value);
        int zerobytes = zerobits >> 3;

        return zerobytes;
    }


    void unsortedArray() noexcept {
        auto map = map_;
        auto limit = map + length_;

        constexpr size_t kMaxRecords = 500;

        int nrecords = 0;
        auto records = new(std::nothrow) Temperature[kMaxRecords];

        size_t count = 0;
        while (map < limit) {
            auto store = records + nrecords;
            map = copyLocation(map, store->location_);
            double temp = popTemperature(map);
            map = findChar(map, 0x0A);
            ++map;

            ++count;
            if ((count % (1000*1000LL)) == 0) {
                LOG("count="<<count<<" nrecords="<<nrecords);
            }

            bool found = false;
            for (int i = 0; i < nrecords; ++i) {
                auto test = records + i;
                if (std::strcmp(test->location_, store->location_) == 0) {
                    found = true;
                    store->min_ = std::min(store->min_, temp);
                    store->max_ = std::max(store->max_, temp);
                    store->sum_ += temp;
                    store->count_++;
                    break;
                }
            }
            if (found == false) {
                ++nrecords;
                store->min_ = temp;
                store->max_ = temp;
                store->sum_ = temp;
                store->count_ = 1;
            }
        }

        for (int i = 0; i < nrecords; ++i) {
            auto &rec = records[i];
            double avg = rec.sum_ / double(rec.count_);
            LOG("tree["<<i<<"]=\""<<rec.location_<<"\";"<<rec.min_<<";"<<rec.max_<<";"<<avg);
        }
        LOG("tree.size="<<nrecords);

        delete[] records;
    }

    double popTemperature(
        char *src
    ) noexcept {
        auto lim = findChar(src, 0x0A);
        auto len = lim - src;
        std::string stemp(src, len);
        double temp = std::stod(stemp);
        return temp;
    }

    void unorderedTree() noexcept {
        auto map = map_;
        auto limit = map + length_;

        std::unordered_map<std::string, Temperature> tree;
        tree.reserve(1000);

        while (map < limit) {
            /** find where. **/
            auto pos = findChar(map, ';');
            auto len = pos - map;
            std::string s(map, len);
            ++pos;

            /** find the temperature. **/
            map = findChar(pos, 0x0A);
            len = map - pos;
            std::string stemp(pos, len);
            double temp = std::stod(stemp);
            ++map;

            /** update the tree. **/
            auto found = tree.find(s);
            if (found == tree.end()) {
                /** add it. **/
                Temperature rec;
                rec.min_ = temp;
                rec.max_ = temp;
                rec.sum_ = temp;
                rec.count_ = 1;
                tree.insert({s, rec});
            } else {
                /** update it. **/
                auto &rec = found->second;
                rec.min_ = std::min(rec.min_, temp);
                rec.max_ = std::max(rec.max_, temp);
                rec.sum_ += temp;
                ++rec.count_;
            }
        }

        int idx = 0;
        for (auto &&iter : tree) {
            auto &loc = iter.first;
            auto &rec = iter.second;
            double avg = rec.sum_ / double(rec.count_);
            LOG("tree["<<idx<<"]=\""<<loc<<"\";"<<rec.min_<<";"<<rec.max_<<";"<<avg);
            ++idx;
        }
        int sz = tree.size();
        LOG("tree.size="<<sz);
    }

    void analyzeTree() noexcept {
        auto map = map_;
        auto limit = map + length_;

        std::unordered_map<std::string, int> tree;
        tree.reserve(1000);

        int longest = 0;
        while (map < limit) {
            auto pos = findChar(map, ';');
            auto len = pos - map;
            std::string s(map, len);
            auto found = tree.find(s);
            if (found == tree.end()) {
                /** how similar is it to an existing entry. **/
                for (auto &&iter : tree) {
                    auto &loc = iter.first;
                    int count = 0;
                    int sz = std::min(s.size(), loc.size());
                    for (int i = 0; i < sz; ++i) {
                        if (s[i] != loc[i]) {
                            break;
                        }
                        ++count;
                    }
                    if (longest < count) {
                        longest = count;
                        LOG("similar names ("<<longest<<"): \""<<s<<"\" and \""<<loc<<"\"");
                    }
                }

                /** add it **/
                tree.insert({s, 0});
            }
            map = findChar(pos+1, 0x0A);
            ++map;
        }

        int min_len = 1000;
        int max_len = 0;
        int idx = 0;
        for (auto &&iter : tree) {
            auto &loc = iter.first;
            LOG("tree["<<idx<<"]=\""<<loc<<"\"");
            ++idx;
            int len = loc.size();
            min_len = std::min(min_len, len);
            max_len = std::max(max_len, len);
        }
        int sz = tree.size();
        LOG("tree.size="<<sz);
        LOG("min_len="<<min_len);
        LOG("max_len="<<max_len);
        LOG("longest="<<longest);
    }

    char *findChar(
        char *pos,
        int what
    ) noexcept {
        for(;;) {
            int ch = *pos;
            if (ch == what) {
                return pos;
            }
            ++pos;
        }
    }

    void analyzeDistribution() noexcept {
        auto table = table_;
        auto map = map_;
        auto length = length_;

        for (int i = 0; i < 27; ++i) {
            table[i] = 0;
        }

        int min_line = 1000;
        int max_line = 0;
        int max_precision = 0;
        int nlines = 0;
        size_t idx = 0;
        while (idx < length) {
            int ch = map[idx];
            ch = std::tolower(ch);
            ch -= 'a';
            if (ch < 0 || ch >= 27) {
                ch = 27-1;
            }
            ++table[ch];
            ++idx;

            size_t precision = 0;
            int line = 1;
            while (idx < length) {
                ch = map[idx];
                ++idx;
                if (ch == 0x0A) {
                    ++nlines;
                    break;
                }
                ++line;
                if (ch == '.') {
                    precision = idx;
                }
            }
            min_line = std::min(min_line, line);
            max_line = std::max(max_line, line);
            if (precision) {
                int prec = idx - precision;
                max_precision = std::max(max_precision, prec);
            }
        }

        size_t sum = 0;
        for (int i = 0; i < 27; ++i) {
            char ch = ' ';
            if (i != 26) {
                ch = i + 'a';
            }
            int count = table[i];
            LOG("table['"<<ch<<"']="<<underscoreFormat(count)<<" sum="<<underscoreFormat(sum));
            sum += count;
        }
        LOG("nlines="<<nlines);
        LOG("min_line="<<min_line);
        LOG("max_line="<<max_line);
        LOG("max_precision="<<max_precision);
    }

    std::string underscoreFormat(
        size_t x
    ) noexcept {
        std::string s = std::to_string(x);
        int len = s.size();
        int where = len - 3;
        while (where > 0) {
            s.insert(where, 1, '_');
            where -= 3;
        }
        return s;
    }
};

int main(
    int argc, char *argv[]
) noexcept {
    (void) argc;
    (void) argv;

    agm::log::init(AGM_TARGET_NAME ".log");
    LOG("One Billion Row Challenge...");

    /**
    this class can be kinda large.
    we don't want it on the stack.
    we want it in the heap.
    **/
    auto obr = new(std::nothrow) OneBillionRows;
    obr->run();
    delete obr;

    LOG("Done!");

    return 0;
}

/** experiments **/
#if 0
    //                         11111111112222222222333333333
    //               012345678901234567890123456789012456789
    std::string str("+++abcdefghiklmnopqrstuvwxyzabcdefghikl");
    const char *src0 = str.c_str() + 3;
    LOG("src0="<<(void *)src0);

    static constexpr int kBuffSize = 32;
    char buff[8*kBuffSize];
    for (int i = 0; i < 8*kBuffSize; ++i) {
        buff[i] = '*';
    }
    char *dst0 = (char *) (agm::uint64(&buff[kBuffSize]) & ~31LL);
    LOG("dst0="<<(void *)dst0);
    const char *lim = dst0 + kBuffSize;
    for (agm::int64 i = 0; i < 1*1000*1000*1000LL; ++i) {
        /**
        must move the delimiter.
        otherwise the compiler unrolls loops.
        and branch prediction becomes perfect.
        **/
        char *poke = (i%28) + 4 + (char *) src0;
        char save = *poke;
        *poke = ';';

        const char *src = src0;
        char *dst = dst0;

#if 0
        /** 5.3s **/
        auto src64 = (agm::int64 *) src;
        auto dst64 = (agm::int64 *) dst;
        dst64[0] = src64[0];
        dst64[1] = src64[1];
        dst64[2] = src64[2];
        dst64[3] = src64[3];
        for (; dst < lim; ++dst) {
            if (*dst == ';') {
                break;
            }
        }
        while (dst < lim) {
            *dst++ = 0;
        }
#elif 0
        /** 5.3s **/
        auto src64 = (agm::int64 *) src;
        auto dst64 = (agm::int64 *) dst;
        auto a = src64[0];
        auto b = src64[1];
        dst64[0] = a;
        dst64[1] = b;
        a = src64[2];
        b = src64[3];
        dst64[2] = a;
        dst64[3] = b;
        for (; dst < lim; ++dst) {
            if (*dst == ';') {
                break;
            }
        }
        while (dst < lim) {
            *dst++ = 0;
        }
#elif 0
        /** 4.8s **/
        auto src64 = (agm::int64 *) src;
        auto dst64 = (agm::int64 *) dst;
        auto x0 = src64[0];
        auto x1 = src64[1];
        auto x2 = src64[2];
        auto x3 = src64[3];
        dst64[0] = x0;
        dst64[1] = x1;
        dst64[2] = x2;
        dst64[3] = x3;
        for (; dst < lim; ++dst) {
            if (*dst == ';') {
                break;
            }
        }
        while (dst < lim) {
            *dst++ = 0;
        }
#elif 0
        /** 25s **/
        char mask = 0xFF;
        while (dst < lim) {
            char ch0 = src[0];
            char ch1 = src[1];
            src += 2;
            /**
            ^ makes semicolon 0.
            -1 makes it negative.
            >> makes it 00 or FF
            ~ makes it FF or 00
            **/
            char x0 = ~(((ch0 ^ ';') - 1) >> 7);
            char x1 = ~(((ch1 ^ ';') - 1) >> 7);
            dst[0] = ch0 & mask & x0;
            dst[1] = ch1 & mask & x0 & x1;
            mask &= x0 & x1;
            dst += 2;
        }
#elif 0
        /** 4.6s **/
        for(;;) {
            int ch = *src++;
            if (ch == ';') {
                break;
            }
            *dst++ = ch;
        }
        while (dst < lim) {
            *dst++ = 0;
        }
#elif 1
        /** 5.3s **/
        auto src64 = (agm::int64 *) src;
        auto dst64 = (agm::int64 *) dst;
        dst64[0] = src64[0];
        dst64[1] = src64[1];
        dst64[2] = src64[2];
        dst64[3] = src64[3];
        for (; dst < lim; ++dst) {
            if (*dst == ';') {
                break;
            }
        }
        int nzeros = lim - dst;
        int rem = nzeros & 7;
        switch (rem) {
            case 7: dst[6] = 0; [[fallthrough]];
            case 6: dst[5] = 0; [[fallthrough]];
            case 5: dst[4] = 0; [[fallthrough]];
            case 4: dst[3] = 0; [[fallthrough]];
            case 3: dst[2] = 0; [[fallthrough]];
            case 2: dst[1] = 0; [[fallthrough]];
            case 1: dst[0] = 0; [[fallthrough]];
            default: ;
        }
        dst += rem;
        while (nzeros >= 8) {
            * (agm::int64 *) dst = 0;
            dst += 8;
            nzeros -= 8;
        }
        /*while (dst < lim) {
            *dst++ = 0;
        }*/
#elif 0
        /** 4.6s **/
        for(;;) {
            int ch = *src++;
            if (ch == ';') {
                break;
            }
            *dst++ = ch;
        }
        switch (lim - dst) {
            case 32: dst[31] = 0; [[fallthrough]];
            case 31: dst[30] = 0; [[fallthrough]];
            case 30: dst[29] = 0; [[fallthrough]];
            case 29: dst[28] = 0; [[fallthrough]];
            case 28: dst[27] = 0; [[fallthrough]];
            case 27: dst[26] = 0; [[fallthrough]];
            case 26: dst[25] = 0; [[fallthrough]];
            case 25: dst[24] = 0; [[fallthrough]];
            case 24: dst[23] = 0; [[fallthrough]];
            case 23: dst[22] = 0; [[fallthrough]];
            case 22: dst[21] = 0; [[fallthrough]];
            case 21: dst[20] = 0; [[fallthrough]];
            case 20: dst[19] = 0; [[fallthrough]];
            case 19: dst[18] = 0; [[fallthrough]];
            case 18: dst[17] = 0; [[fallthrough]];
            case 17: dst[16] = 0; [[fallthrough]];
            case 16: dst[15] = 0; [[fallthrough]];
            case 15: dst[14] = 0; [[fallthrough]];
            case 14: dst[13] = 0; [[fallthrough]];
            case 13: dst[12] = 0; [[fallthrough]];
            case 12: dst[11] = 0; [[fallthrough]];
            case 11: dst[10] = 0; [[fallthrough]];
            case 10: dst[9] = 0; [[fallthrough]];
            case 9: dst[8] = 0; [[fallthrough]];
            case 8: dst[7] = 0; [[fallthrough]];
            case 7: dst[6] = 0; [[fallthrough]];
            case 6: dst[5] = 0; [[fallthrough]];
            case 5: dst[4] = 0; [[fallthrough]];
            case 4: dst[3] = 0; [[fallthrough]];
            case 3: dst[2] = 0; [[fallthrough]];
            case 2: dst[1] = 0; [[fallthrough]];
            case 1: dst[0] = 0; [[fallthrough]];
            default: ;
        }
#elif 1
        /** 4.6s **/
        for(;;) {
            int ch = *src++;
            if (ch == ';') {
                break;
            }
            *dst++ = ch;
        }
        int nzeros = lim - dst;
        /*switch (nzeros >> 3) {
        case 3:
            ((agm::int64 *) dst)[0] = 0;
            ((agm::int64 *) dst)[1] = 0;
            ((agm::int64 *) dst)[2] = 0;
            break;
        case 2:
            ((agm::int64 *) dst)[0] = 0;
            ((agm::int64 *) dst)[1] = 0;
            break;
        case 1:
            ((agm::int64 *) dst)[0] = 0;
            break;
        default: ;
        }*/
        while (nzeros >= 8) {
            * (agm::int64 *) dst = 0;
            dst += 8;
            nzeros -= 8;
        }
        switch (nzeros & 7) {
            case 7: dst[6] = 0; [[fallthrough]];
            case 6: dst[5] = 0; [[fallthrough]];
            case 5: dst[4] = 0; [[fallthrough]];
            case 4: dst[3] = 0; [[fallthrough]];
            case 3: dst[2] = 0; [[fallthrough]];
            case 2: dst[1] = 0; [[fallthrough]];
            case 1: dst[0] = 0; [[fallthrough]];
            default: ;
        }
#else
        /** 4.6s **/
        for(;;) {
            int ch = *src++;
            if (ch == ';') {
                break;
            }
            *dst++ = ch;
        }
        int nzeros = lim - dst;
        switch (nzeros) {
            case 32: dst[31] = 0; [[fallthrough]];
            case 31: dst[30] = 0; [[fallthrough]];
            case 30: dst[29] = 0; [[fallthrough]];
            case 29: dst[28] = 0; [[fallthrough]];
            case 28: dst[27] = 0; [[fallthrough]];
            case 27: dst[26] = 0; [[fallthrough]];
            case 26: dst[25] = 0; [[fallthrough]];
            case 25: dst[24] = 0; [[fallthrough]];
            case 24: dst[23] = 0; [[fallthrough]];
            case 23: dst[22] = 0; [[fallthrough]];
            case 22: dst[21] = 0; [[fallthrough]];
            case 21: dst[20] = 0; [[fallthrough]];
            case 20: dst[19] = 0; [[fallthrough]];
            case 19: dst[18] = 0; [[fallthrough]];
            case 18: dst[17] = 0; [[fallthrough]];
            case 17: dst[16] = 0; [[fallthrough]];
            case 16: dst[15] = 0; [[fallthrough]];
            case 15: dst[14] = 0; [[fallthrough]];
            case 14: dst[13] = 0; [[fallthrough]];
            case 13: dst[12] = 0; [[fallthrough]];
            case 12: dst[11] = 0; [[fallthrough]];
            case 11: dst[10] = 0; [[fallthrough]];
            case 10: dst[9] = 0; [[fallthrough]];
            case 9: dst[8] = 0; [[fallthrough]];
            case 8: dst[7] = 0; [[fallthrough]];
            case 7: dst[6] = 0; [[fallthrough]];
            case 6: dst[5] = 0; [[fallthrough]];
            case 5: dst[4] = 0; [[fallthrough]];
            case 4: dst[3] = 0; [[fallthrough]];
            case 3: dst[2] = 0; [[fallthrough]];
            case 2: dst[1] = 0; [[fallthrough]];
            case 1: dst[0] = 0; [[fallthrough]];
            default: ;
        }
#endif
        *poke = save;
    }
    for (int i = 0; i < 8*kBuffSize; ++i) {
        if (buff[i] == 0) {
            buff[i] = '.';
        }
    }
    std::string result(dst0, kBuffSize);
    LOG("result=\""<<result<<"\"");
    return 0;
#endif
