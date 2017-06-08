//
// Created by roman on 07.06.17.
//

#include <cstdio>
#include <emmintrin.h>
#include <memory>
#include <vector>
#include <iostream>
#include <chrono>
#include <cstring>
#include <cassert>
#ifdef DEBUG
#define FUN std::cout << __func__ << '\n';
#define DB(x) std::cout << #x << " = " << x << '\n';
#else
#define FUN
#define DB(x)
#endif


const size_t align = sizeof(__m128i);

void vectorized_memcpy(void *dst, const void *src, size_t count) {
    size_t vectorization_begin = 0;
    const char *char_src = static_cast<const char *>(src);
    char *char_dst = static_cast<char *> (dst);
    for (size_t i = 0;; i++) {
        if ((i >= count) || ((reinterpret_cast<size_t >(dst) + i) % align == 0)) {
            vectorization_begin = i;
            break;
        }
        *(char_dst + i) = *(char_src + i);
    }
    size_t vectorization_length = ((count - vectorization_begin) / align) * align;
    size_t vectorization_end = vectorization_begin + vectorization_length;
    DB(vectorization_begin)
    DB(vectorization_end)
    DB(vectorization_length)
    char_src += vectorization_begin;
    char_dst += vectorization_begin;
//    for (size_t i = vectorization_begin; i + align <= count; i += align) {
        __m128i xmm_tmp;
        __asm__ volatile (
        "0:"
                "cmp $0, %3\n\t"
                "jne 1f\n\t"
                "jmp 2f\n\t"
        "1:"
                "movdqu (%1), %0\n\t"
                "movntdq %0, (%2)\n\t"
                "add $16, %1\n\t"
                "add $16, %2\n\t"
                "sub $16, %3\n\t"
                "jnz 1b\n\t"
        "2:"
                "nop"
        :"=x"(xmm_tmp), "=r"(char_src), "=r"(char_dst), "=r"(vectorization_length)
        :"1"(char_src), "2"(char_dst), "3"(vectorization_length)
        );
//    }
    for (size_t i = 0; i < count - vectorization_end; ++i) {
        *(char_dst + i) = *(char_src + i);
    }
}

bool check_equality(const void *ptr, const void *a_ptr, size_t sz) {
    for (size_t i = 0; i < sz; ++i)
        if (*(static_cast<const char *>(ptr) + i) != *(static_cast<const char *>(a_ptr) + i))
            return false;
    return true;
}


bool is_aligned(void const* ptr, size_t alignment)
{
    return (reinterpret_cast<size_t>(ptr) % alignment) == 0;
}

void copy_nt_asm(void* dst, void const* src, size_t N)
{
    assert(is_aligned(dst, sizeof(__m128i)));
    assert(is_aligned(src, sizeof(__m128i)));
    assert((N % sizeof(__m128i)) == 0);

    __m128i tmp;
    __asm__ volatile(
    "1:"
            "movdqa     (%0), %3\n"
            "movntdq    %3, (%1)\n"
            "add        $16, %0\n"
            "add        $16, %1\n"
            "sub        $16, %2\n"
            "jnz        1b\n"
    : "=r"(dst), "=r"(src), "=r"(N), "=x"(tmp)
    : "0"(dst),  "1"(src),  "2"(N)
    : "memory", "cc"
    );
}

void test_correctness() {
    const size_t size = 1000;
    size_t first_count = size;
    void *first_allocated = malloc(1024);
    void *copy_from = std::align(align, size, first_allocated, first_count);
    if (!copy_from) {
        printf("failed to align copy_from\n");
        return;
    }
    size_t second_count = size;
    void *second_allocated = malloc(1024);
    void *copy_to = std::align(align, size, second_allocated, first_count);
    if (!copy_to) {
        printf("failed to align copy_to\n");
        return;
    }
    std::vector<std::pair<size_t, size_t>> tests;
    size_t max_count = std::min(first_count, second_count);
    tests.emplace_back(0, 0);
    tests.emplace_back(0, align - 1);
    tests.emplace_back(0, 2 * align - 1);
    tests.emplace_back(0, max_count);
    for (size_t offset = 1; offset < align; ++offset) {
        tests.emplace_back(offset, align - offset);
        tests.emplace_back(offset, 5 * align - offset);
        tests.emplace_back(offset, 2 * align - offset + 1);
    }
    printf("Testing...\n");
    for (size_t i = 7; i < tests.size(); ++i) {
        auto t = tests[i];
        void *dst = static_cast<void *> (static_cast<char *>(copy_to) + t.first);
        size_t count = t.second;
        vectorized_memcpy(dst, copy_from, count);
        if (!check_equality(dst, copy_from, count)) {
            printf("wrong answer %lu, offset = %lu, count = %lu\n", i + 1, t.first, count);
            break;
        } else printf("done test %lu\n", i + 1);
    }
    printf("OK\n");
    free(first_allocated);
    free(second_allocated);
}

void test_perfomance() {
    const size_t N = 100000000;
    std::vector<char> a(N), b(N);
    auto start = std::chrono::high_resolution_clock::now();
    memcpy(a.data(), b.data(), N);
    auto finish = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = finish - start;
    printf("memcpy took %lf s\n", elapsed.count());
    start = std::chrono::high_resolution_clock::now();
    vectorized_memcpy(a.data(), b.data(), N);
    finish = std::chrono::high_resolution_clock::now();
    elapsed = finish - start;
    printf("vectorized_memcpy took %lf s\n", elapsed.count());
}

void test_nt() {
    const size_t N = 100000000;
    std::vector<char> a(N), b(N);
    copy_nt_asm(a.data(), b.data(), 0);
    printf("OK\n");
}

int main() {
//    test_nt();
    test_correctness();
    test_perfomance();
    return 0;
}