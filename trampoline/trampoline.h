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


template<typename R, typename... Args>
struct trampoline<R(Args...)> {


    const unsigned char shift_opcodes[5][3] = {
            {0x4D, 0x89, 0xC1}, // mov r9, r8
            {0x49, 0x89, 0xC8},
            {0x48, 0x89, 0xD1},
            {0x48, 0x89, 0xF2},
            {0x48, 0x89, 0xFE} // mov rsi, rdi
    };


    const std::string stack_opcodes[23] = {
// 19:  push r9
            "\x41\x51",

// 1B:  push r12
            "\x41\x54",

// 1D:  lea r12,[rsp+imm]
            "\x4C\x8D\xA4\x24",

// 25:  lea rax,[rsp+0x8]
            "\x48\x8D\x44\x24\x08",

// 2A:  mov r11,[rsp+0x8]

            "\x4C\x8B\x5C\x24\x08",

// 2F:  cmp r12,rax
            "\x49\x39\xC4",

// 32:  jz 0x41
            "\x74\x0D",

// 34:  mov r10,[rax+0x8]
            "\x4C\x8B\x50\x08",

// 38:  mov [rax],r10
            "\x4C\x89\x10",

// 3B:  add rax,byte +0x8
            "\x48\x83\xC0\x08",

// 3F:  jmp short 0x2f
            "\xEB\xEE",

// 41:  mov [r12],r11
            "\x4D\x89\x1C\x24",

// 45:  mov r11,[rsp+0x8]
            "\x4C\x8B\x5C\x24\x08",

// 4A:  lea r12,[rsp+imm2]
            "\x4C\x8D\xA4\x24",

// 52:  lea rax,[rsp+0x8]
            "\x48\x8D\x44\x24\x08",

// 57:  cmp r12,rax
            "\x49\x39\xC4",

// 5A:  jz 0x69
            "\x74\x0D",

// 5C:  mov r10,[rax+0x8]
            "\x4C\x8B\x50\x08",

// 60:  mov [rax],r10
            "\x4C\x89\x10",

// 63:  add rax,byte +0x8
            "\x48\x83\xC0\x08",

// 67:  jmp short 0x57
            "\xEB\xEE",

// 69:  mov [r12],r11
            "\x4D\x89\x1C\x24",

// 6D:  pop r12
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

    void write_instr(const std::string &instr, unsigned char *&cur_code) {
        for (size_t j = 0; j < instr.length(); ++j)
            *cur_code++ = static_cast<unsigned char>(instr[j]);
    }


    std::function<void(void *)> code_del = [](void *ptr) { allocator::free(ptr); };


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
