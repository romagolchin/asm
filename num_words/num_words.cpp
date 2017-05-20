#include <cstdio>
#include <cstdlib>
#include <string>
#include <cstring>
#include <iostream>
#include <memory>
#include <cassert>
#include <emmintrin.h>
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

    __m128i res = _mm_set_epi32(0, 0, 0, 0);
    __m128i sum = _mm_set_epi32(0, 0, 0, 0);
    size_t limit = (len - offset) / 16;
    for (size_t i = 0, j = offset + 16 * i; j + 15 < len; ++i, j += 16) {
        __m128i str_fragment = _mm_loadu_si128(
                reinterpret_cast<const __m128i *>(str + j)
        );
        str_fragment = _mm_cmpeq_epi8(str_fragment, reg_spaces);
        __m128i shifted_str_fragment = _mm_slli_si128(str_fragment, 1);
        if (i > 0 && str[j - 1] == ' ')
            shifted_str_fragment = _mm_or_si128(shifted_str_fragment, _mm_set_epi32(0, 0, 0, 0xff));
        // set least significant bit to 1 of each byte iff str_fragment has space at this position and
        // shifted_str_fragment hasn't
        __m128i count = _mm_and_si128(_mm_andnot_si128(str_fragment, shifted_str_fragment), mask);
        sum = _mm_add_epi8(sum, count);
        if ((i == limit - 1) || ((i + 1) % 255 == 0)) {
            // popcount of sum in log log time
            sum = _mm_add_epi16(
                    _mm_and_si128(sum, _mm_set_epi32(0x00ff00ff, 0x00ff00ff, 0x00ff00ff, 0x00ff00ff)),
                    _mm_bsrli_si128(_mm_and_si128(sum, _mm_set_epi32(0xff00ff00, 0xff00ff00, 0xff00ff00, 0xff00ff00)),
                                    1)
            );
            sum = _mm_add_epi32(
                    _mm_and_si128(sum, _mm_set_epi32(0x0000ffff, 0x0000ffff, 0x0000ffff, 0x0000ffff)),
                    _mm_bsrli_si128(_mm_and_si128(sum, _mm_set_epi32(0xffff0000, 0xffff0000, 0xffff0000, 0xffff0000)),
                                    2)
            );
            sum = _mm_add_epi64(
                    _mm_and_si128(sum, _mm_set_epi32(0, 0xffffffff, 0, 0xffffffff)),
                    _mm_bsrli_si128(_mm_and_si128(sum, _mm_set_epi32(0xffffffff, 0, 0xffffffff, 0)), 4)
            );
            sum = _mm_add_epi64(
                    _mm_and_si128(sum, _mm_set_epi32(0, 0, 0xffffffff, 0xffffffff)),
                    _mm_bsrli_si128(_mm_and_si128(sum, _mm_set_epi32(0xffffffff, 0xffffffff, 0, 0)), 8)
            );
            // add sum to res, set sum to zero
            res = _mm_add_epi64(res, sum);
            sum = _mm_set_epi32(0, 0, 0, 0);
        }
    }

    for (size_t i = offset + ((len - offset) / 16) * 16; i + 1 < len; ++i)
        add_res += (str[i] == ' ' && str[i + 1] != ' ');

    union {
        __m128i reg;
        size_t a[2];
    } nw;
    _mm_storel_epi64(&nw.reg, res);

    return nw.a[0] + add_res;
}


int main() {
    srand(time(0));
    std::vector<std::string> test;
    std::vector<size_t> ind;
    test.push_back("aba caba baba");
    test.push_back("aaa ");
    test.push_back("Running this 3459 times...");
    test.push_back("\0");
    test.push_back("  ");
    test.push_back(" aa ");
    test.push_back(" aa gg wwwww ");
    test.push_back("aa ");
    test.push_back("aa");
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
    for (std::string t : test) {
        ind.push_back(t.empty() ? 0 : rand() % t.length());
    }
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
    std::cout << std::endl;
    std::chrono::duration<double> asm_elapsed = after - before;
    std::cout << "Elapsed time (asm): " << asm_elapsed.count() << "s\n";
    std::cout << elapsed / asm_elapsed << " times faster" << std::endl;
    for (size_t it = 0; it < answers.size(); ++it) {
        if (answers[it] != asm_answers[it]) {
            printf("'%s'\n", test[it].substr(ind[it]).c_str());
            std::cout << answers[it] << ' ' << asm_answers[it] << std::endl;
            std::cout << it << std::endl;
            assert(false);
        }
    }

    return 0;
}