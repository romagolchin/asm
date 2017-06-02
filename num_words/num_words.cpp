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

const int N = 1 << 15;
const int I = 1 << 10;
const int K = 1 << 12;


void gen() {
    srand(time(0));
    std::ofstream out("test");
    for (int i = 0; i < I; ++i) {
        std::string t;
        for (int j = 0; j < N; ++j) {
            int x = rand() % 3;
            t += x == 0 ? ' ' : ('a' + x - 1);
        }
        out << (t + "\n");
    }
    for (int i = 0; i < K; ++i)
        out << "a ";
    out << '\n';
    for (int i = 0; i < K; ++i) {
        out << " a";
    }
    out << '\n';
    out.close();
}


size_t num_words(const std::string &s) {
    char const *str = s.c_str();
    size_t res = 0;
    if ((*str != ' ') && *str)
        res++;
    for (size_t i = 0; str[i] && str[i + 1]; ++i)
        res += (str[i] == ' ' && str[i + 1] != ' ');
    return res;
}


const __m128i reg_spaces = _mm_set_epi8(32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32);

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

void test(int times = 100) {
    bool ok = true;
    double sum = 0.;
    double asm_sum = 0.;
    for (int it = 0; it < times && ok; ++it) {
        gen();
        std::vector<std::string> test;
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
        auto before = std::chrono::high_resolution_clock::now();
        for (size_t i = 0; i < test.size(); ++i) {
            std::string t = test[i];
            answers.push_back(num_words(t));
        }
        auto after = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = after - before;
        sum += elapsed.count();
        std::cout << "Elapsed time (straightforward): " << elapsed.count() << "s\n";
        before = std::chrono::high_resolution_clock::now();
        for (size_t i = 0; i < test.size(); ++i) {
            std::string t = test[i];
            asm_answers.push_back(asm_num_words(t));
        }
        after = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> asm_elapsed = after - before;
        asm_sum += asm_elapsed.count();
        std::cout << "Elapsed time (asm): " << asm_elapsed.count() << "s\n";
        if (asm_elapsed.count() >= 1e-9)
            std::cout << elapsed / asm_elapsed << " times faster" << std::endl;
        for (size_t j = 0; j < answers.size(); ++j) {
            if (answers[j] != asm_answers[j]) {
                std::cout << answers[j] << ' ' << asm_answers[j] << std::endl;
                printf("'%s'\n", test[j].c_str());
                std::cout << j << std::endl;
                ok = false;
                break;
            }
        }
        if (it != 0 && it % 10 == 0)
            printf("%d tests passed\n", it + 1);
        if (it == times - 1 || !ok)
            printf("on average : %lf ", sum / asm_sum);
    }
}


int main() {
    test();
    return 0;
}