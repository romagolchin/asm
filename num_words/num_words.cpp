#include <cstdio>
#include <cstdlib>
#include <string>
#include <cstring>
#include <iostream>
#include <memory>
#include <cassert>
#include <emmintrin.h>
#include <immintrin.h>
#include <assert.h>
#include <random>
#include <ctime>
#include <chrono>
#include <fstream>



size_t num_words(const std::string &s) {
    char const *str = s.c_str();
    size_t res = 0;
    if ((*str != ' ') && *str)
        res++;
    for (size_t i = 0; str[i] && str[i + 1]; ++i)
        res += (str[i] == ' ' && str[i + 1] != ' ');
    return res;
}


__m128i mask = _mm_set_epi32(0x01010101, 0x01010101, 0x01010101, 0x01010101);
std::string string(16, ' ');
const char *spaces = string.c_str();
__m128i reg_spaces = _mm_loadu_si128(
        reinterpret_cast<const __m128i *>(spaces)
);

void print_128i(__m128i reg) {
    union {
        __m128i r;
        size_t ar[2];
    } u;
    u.r = reg;
    printf("%016llx%016llx\n", u.ar[1], u.ar[0]);
}


size_t asm_num_words(const std::string &s) {
    const size_t len = s.length();
    const char *str = s.c_str();
    size_t add_res = 0;
    if (len > 0 && str[0] != ' ')
        add_res++;
    size_t offset = len > 0 ? len - 1 : 0;
    for (size_t i = 0; i + 1 < len; ++i) {
        if (( (size_t) (str + i)) % 16 == 0) {
            offset = i;
            break;
        }
        add_res += (str[i] == ' ' && str[i + 1] != ' ');
    }
    size_t R = 0;
    for (size_t i = 0, j = offset + 16 * i; j + 15 < len; ++i, j += 16) {
        __m128i str_fragment = _mm_loadu_si128(
                reinterpret_cast<const __m128i *>(str + j)
        );
        str_fragment = _mm_cmpeq_epi8(str_fragment, reg_spaces);
        __m128i shifted_str_fragment = _mm_bsrli_si128(str_fragment, 1);
        if (j + 16 >= len || str[j + 16] == ' ')
            shifted_str_fragment = _mm_or_si128(shifted_str_fragment, _mm_set_epi32(0xff000000, 0, 0, 0));
        // set all bits in byte iff str_fragment has space at this position and
        // shifted_str_fragment hasn't
        __m128i words_positions = _mm_andnot_si128(shifted_str_fragment, str_fragment);
        int count = _popcnt32(_mm_movemask_epi8(words_positions));
        R += count;
    }
    for (size_t i = offset + ((len - offset) / 16) * 16; i + 1 < len; ++i)
        add_res += (str[i] == ' ' && str[i + 1] != ' ');
    return R + add_res;
}


int main() {
    srand(time(0));
    std::vector<std::string> test;
    test.push_back("");
    std::vector<size_t> ind;
    std::vector<size_t> answers;
    std::vector<size_t> asm_answers;
    std::ifstream in("test");
    std::string s;
    if (in.is_open()) {
        while (!in.eof()) {
            std::getline(in, s);
            test.push_back(s);
        }
        in.close();
    }
//    for (std::string t : test) {
//        ind.push_back(t.empty() ? 0 : rand() % t.length());
//    }
    auto before = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < test.size(); ++i) {
        std::string t = test[i];
        answers.push_back(num_words(t));
    }
    auto after = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = after - before;
    std::cout << "Elapsed time (straightforward): " << elapsed.count() << "s\n";
    before = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < test.size(); ++i) {
        std::string t = test[i];
        asm_answers.push_back(asm_num_words(t));
    }
    after = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> asm_elapsed = after - before;
    std::cout << "Elapsed time (asm): " << asm_elapsed.count() << "s\n";
    if (asm_elapsed.count() >= 1e-9)
    std::cout << elapsed / asm_elapsed << " times faster" << std::endl;
    for (size_t it = 0; it < answers.size(); ++it) {
        if (answers[it] != asm_answers[it]) {
            std::cout << answers[it] << ' ' << asm_answers[it] << std::endl;
            printf("'%s'\n", test[it].c_str());
            std::cout << it << std::endl;
//            assert(false);
        }
    }

    return 0;
}