#include <cstdio>
#include <cstdlib>
#include <string>
#include <cstring>
#include <iostream>
#include <memory>
#include <cassert>
#include <emmintrin.h>
#include <immintrin.h>
#include <tmmintrin.h>
#include <assert.h>
#include <random>
#include <ctime>
#include <chrono>
#include <fstream>
#ifdef DEBUG
#define FUN std::cout << __func__ << '\n';
#define DB(x) std::cout << #x << " = " << x << '\n';
#else
#define FUN
#define DB(x)
#endif

const int N = static_cast<int>(1e7);
const int K = 1 << 12;


void gen(size_t cnt) {
    FUN
    srand(time(0));
    std::ofstream out("test");
    for (int i = 0; i < cnt; ++i) {
        std::string t;
        for (int j = 0; j < N; ++j) {
            int x = rand() % 6;
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
    FUN
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
    printf("%016lx%016lx\n", u.ar[1], u.ar[0]);
}


size_t asm_num_words(const std::string &s) {
    const size_t len = s.length();
    const char *str = s.c_str();
    size_t add_res = 0;
    if (len > 0 && str[0] != ' ')
        add_res++;
    size_t offset = len > 0 ? len - 1 : 0;
    for (size_t i = 0; i + 1 < len; ++i) {
        if (((size_t) (str + i)) % 16 == 0) {
            offset = i;
            break;
        }
        add_res += (str[i] == ' ' && str[i + 1] != ' ');
    }
    size_t R = 0;
    __m128i str_fragment, next_str_fragment;

    next_str_fragment = _mm_loadu_si128(reinterpret_cast<const __m128i *>(str + offset));
    const size_t vectorization_length = ((len - offset) / 16) * 16;
    size_t vectorization_end = offset + (vectorization_length == 0 ? 0 : vectorization_length - 16);
    for (size_t i = 0, j = offset + 16 * i; j + 31 < len; ++i, j += 16) {
        str_fragment = next_str_fragment;
        next_str_fragment = _mm_loadu_si128(reinterpret_cast<const __m128i*>(str + j + 16));
        __m128i shifted_str_fragment = _mm_alignr_epi8(next_str_fragment, str_fragment, 1);
        str_fragment = _mm_cmpeq_epi8(str_fragment, reg_spaces);
        shifted_str_fragment = _mm_cmpeq_epi8(shifted_str_fragment, reg_spaces);
//         set all bits in byte iff str_fragment has space at this position and
//         shifted_str_fragment hasn't
        __m128i words_positions = _mm_andnot_si128(shifted_str_fragment, str_fragment);
       int count = _popcnt32(_mm_movemask_epi8(words_positions));
       R += count;
    }
    for (size_t i = vectorization_end; i + 1 < len; ++i)
        add_res += (str[i] == ' ' && str[i + 1] != ' ');
    return R + add_res;
}

void test(size_t times = 10) {
    bool ok = true;
    double sum = 0.;
    double asm_sum = 0.;
    std::ifstream in("test");
    if (!in.is_open()) {
        in.close();
        gen(times);
        in.open("test");
    }
    std::vector<std::string> test;
    std::vector<size_t> answers;
    std::vector<size_t> asm_answers;
    for (int it = 0; it < times; ++it) {
        std::string s;
        std::getline(in, s);
        test.push_back(s);
    }
    auto before = std::chrono::high_resolution_clock::now();
    for (std::string t : test) {
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
    for (size_t j = 0; j < answers.size(); ++j) {
        std::cout << answers[j] << ' ' << asm_answers[j] << std::endl;
        if (answers[j] != asm_answers[j]) {
            // printf("'%s'\n", test[j].c_str());
            std::cout << j << std::endl;
            ok = false;
            break;
        }
    }
    if (ok) {
        printf("OK\n");
    }
    printf("on average : straightforward %lf, asm %lf", sum / times, asm_sum / times);
    if (asm_sum >= 1e-9)
        printf(", %lf times faster", sum / asm_sum);
    printf("\n");
}


int main() {
    test();
    return 0;
}