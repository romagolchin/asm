//
// Created by roman on 03.07.17.
//

#ifndef TRAMPOLINE_H
#define TRAMPOLINE_H

#include <iostream>
#include <unistd.h>
#include <sys/mman.h>
#include <cstdio>
#include <mmintrin.h>
#include <memory>
#include <functional>
#include "allocator.h"
#include "arg_classes.h"
#define DEBUG 1
#ifdef DEBUG
#define FUN (std::cout << __func__ << '\n')
#define DB(x) (std::cout << #x << " = " << x << '\n')
#else
#define FUN
#define DB(x)
#endif


template<typename T>
struct trampoline {
    template<typename F>
    trampoline(F func) {}

    ~trampoline();

    T *get() const;
};



const unsigned char shift_opcodes[5][3] = {
        {0x4D, 0x89, 0xC1}, // mov r9, r8
        {0x49, 0x89, 0xC8},
        {0x48, 0x89, 0xD1},
        {0x48, 0x89, 0xF2},
        {0x48, 0x89, 0xFE} // mov rsi, rdi
};


const std::string stack_opcodes[23] = {
        "\x41\x51",
        "\x41\x54",
        "\x4C\x8D\xA4\x24",
        "\x48\x8D\x44\x24\x08",
        "\x49\x39\xC4",
        "\x74\x14",
        "\x4C\x8B\x10",
        "\x4C\x8B\x58\x08",
        "\x4C\x89\x18",
        "\x4C\x89\x50\x08",
        "\x48\x83\xC0\x08",
        "\xEB\xE7",
        "\x4C\x8B\x5C\x24\x08",
        "\x4C\x8D\xA4\x24",
        "\x48\x8D\x44\x24\x08",
        "\x49\x39\xC4",
        "\x74\x0D",
        "\x4C\x8B\x50\x08",
        "\x4C\x89\x10",
        "\x48\x83\xC0\x08",
        "\xEB\xEE",
        "\x4D\x89\x1C\x24",
        "\x41\x5C"
};


const std::string cleanup_opcodes[18] = {
        // mov r11,[rsp+imm]
        "\x4C\x8B\x9C\x24",

// lea rax,[rsp+imm]
        "\x48\x8D\x84\x24",

// cmp rax,rsp
        "\x48\x39\xE0",

// jz 0x8f
        "\x74\x0D",

// mov r10,[rax-8]
        "\x4C\x8B\x50\xF8",

// mov [rax],r10
        "\x4C\x89\x10",

// sub rax,byte +0x8
        "\x48\x83\xE8\x08",

// jmp short 0x7d
        "\xEB\xEE",

// mov [rsp],r11
        "\x4C\x89\x1C\x24",

// lea rax,[rsp+imm]
        "\x48\x8D\x84\x24",

// cmp rax,rsp
        "\x48\x39\xE0",

// jz 0xad
        "\x74\x0D",

// mov r10,[rax-8]
        "\x4C\x8B\x50\xF8",

// mov [rax],r10
        "\x4C\x89\x10",

// sub rax,byte +0x8
        "\x48\x83\xE8\x08",

// jmp short 0x9b
        "\xEB\xEE",
        // pop r10
"\x41\x5A",
// ret
        "\xC3",


};

void write_instr(const std::string &instr, unsigned char* &cur_code) {
    for (size_t j = 0; j < instr.length(); ++j)
        *cur_code++ = static_cast<unsigned char>(instr[j]);
}


std::function<void(void *)> code_del = [](void *ptr){ std::cout << "!\n"; allocator::free(ptr); };


template<typename R, typename... Args>
struct trampoline<R(Args...)> {

    const int ALL_ARGS = sizeof...(Args);
    typedef typename rev<Args...>::ret args;
    const int SSE_ARGS = args::SSE;
    const int INT_ARGS = ALL_ARGS - SSE_ARGS;


    void shift_args_in_regs(unsigned char *&cur_code) {
        for (int i = std::max(5 - INT_ARGS, 0); i < 5; ++i)
            for (int j = 0; j < 3; ++j)
                *cur_code++ = shift_opcodes[i][j];
    }


    template<typename F>
    trampoline(F const &f) : fun_ptr(std::make_shared<std::function<R(Args...)>>(f)) {
        code_ptr = std::shared_ptr<void>(allocator::alloc(), code_del);
        unsigned char *cur_code = (unsigned char *) code_ptr.get();
        DB(INT_ARGS);
        DB(SSE_ARGS);
        DB(args::SSE_BEFORE_6_INT);
        int n_sse_before_6int_stack = std::max(0, args::SSE_BEFORE_6_INT - 8);
        int n_arg_stack = std::max(0, INT_ARGS - 6) + std::max(0, SSE_ARGS - 8) + 1;
        DB(n_arg_stack);
        if (INT_ARGS < 6) {
            shift_args_in_regs(cur_code);

        } else {
            // 58                pop rax
//            *cur_code++ = 0x5B;
            // 4151              push r9
//            *cur_code++ = 0x41;
//            *cur_code++ = 0x51;
            // 50                push rax
//            *cur_code++ = 0x53;

            // 4151              push r9
            // 4154              push r12
            // 4C8DA42434120000  lea r12,[rsp+0x1234]
            for (int i = 0; i < 3; ++i)
                write_instr(stack_opcodes[i], cur_code);
            *reinterpret_cast<int *>(cur_code) = 8 * (n_sse_before_6int_stack + 2);
            cur_code += 4;
            for (int i = 3; i < 14; ++i)
                write_instr(stack_opcodes[i], cur_code);
            *reinterpret_cast<int *>(cur_code) = 8 * (n_arg_stack + 1);
            cur_code += 4;
            for (int i = 14; i < 23; ++i)
                write_instr(stack_opcodes[i], cur_code);
            // 4889E0            mov rax,rsp
            // 4939C4            cmp r12,rax
            // 7414              jz 0x41
            // 4C8B10            mov r10,[rax]
            // 4C8B5808          mov r11,[rax+0x8]
            // 4C8918            mov [rax],r11
            // 4C895008          mov [rax+0x8],r10
            // 4883C008          add rax,byte +0x8
            // EBE7              jmp short 0x28
            // 415C              pop r12
            shift_args_in_regs(cur_code);
        }
        // 48BF         mov rdi, imm
        *cur_code++ = 0x48;
        *cur_code++ = 0xbf;
        *(void **) cur_code = (void *) fun_ptr.get();
        cur_code += 8;
        // 48B8         mov rax, imm
        *cur_code++ = 0x48;
        *cur_code++ = 0xb8;
        *(void **) cur_code = (void *) &do_call<F>;
        cur_code += 8;
        if (INT_ARGS < 6) {
            // FFE0         jmp rax
            *cur_code++ = 0xFF;
            *cur_code++ = 0xE0;
        } else {
            // FFD0         call rax
            *cur_code++ = 0xFF;
            *cur_code++ = 0xD0;
            for (int i = 0; i < 2; ++i) {
                write_instr(cleanup_opcodes[i], cur_code);
                *reinterpret_cast<int *>(cur_code) = 8 * (n_arg_stack);
                cur_code += 4;
            }
            for (int i = 2; i < 10; ++i) {
                write_instr(cleanup_opcodes[i], cur_code);
            }
            *reinterpret_cast<int *>(cur_code) = 8 * (n_sse_before_6int_stack + 1);
            cur_code += 4;
            for (int j = 10; j < 18; ++j) {
                write_instr(cleanup_opcodes[j], cur_code);
            }
        }
    }

    template<typename F>
    static R do_call(void *f, Args... args) {
        return (*reinterpret_cast<F *>(f))(args...);
    }

    R (*get() const )(Args...) {
        return reinterpret_cast<R (*)(Args...)>(code_ptr.get());
    }



private:

    std::shared_ptr<void> code_ptr;
    std::shared_ptr<std::function<R(Args...)>> fun_ptr;
};

#endif // TRAMPOLINE_H
