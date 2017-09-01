//
// Created by roman on 07.07.17.
//

#ifndef ARG_CLASSES_H
#define ARG_CLASSES_H

template<typename... Args>
struct arg_classes {
};

template<>
struct arg_classes<> {
    static const int SSE_BEFORE_6_INT = 0;
    static const int INT_OR_PTR = 0;
    static const int SSE = 0;
};

template<typename... Rest>
struct arg_classes<float, Rest...> {
    static const int SSE_BEFORE_6_INT = arg_classes<Rest...>::SSE_BEFORE_6_INT;
    static const int INT_OR_PTR = arg_classes<Rest...>::INT_OR_PTR;
    static const int SSE = arg_classes<Rest...>::SSE + 1;
};

template<typename... Rest>
struct arg_classes<double, Rest...> {
    static const int SSE_BEFORE_6_INT = arg_classes<Rest...>::SSE_BEFORE_6_INT;
    static const int INT_OR_PTR = arg_classes<Rest...>::INT_OR_PTR;
    static const int SSE = arg_classes<Rest...>::SSE + 1;
};


template<typename... Rest>
struct arg_classes<__m64, Rest...> {
    static const int SSE_BEFORE_6_INT = arg_classes<Rest...>::SSE_BEFORE_6_INT;
    static const int INT_OR_PTR = arg_classes<Rest...>::INT_OR_PTR;
    static const int SSE = arg_classes<Rest...>::SSE + 1;
};


template<typename First, typename... Rest>
struct arg_classes<First, Rest...> {
    static const int SSE_BEFORE_6_INT =
            arg_classes<Rest...>::INT_OR_PTR == 5 ? arg_classes<Rest...>::SSE : arg_classes<Rest...>::SSE_BEFORE_6_INT;
    static const int INT_OR_PTR = arg_classes<Rest...>::INT_OR_PTR + 1;
    static const int SSE = arg_classes<Rest...>::SSE;
};

template<typename S, typename T>
struct helper {
};

// helper acc [] = acc
template<typename... Acc_args>
struct helper<arg_classes<Acc_args...>, arg_classes<>> {
    typedef arg_classes<Acc_args...> ret;
};

// helper acc (x:xs) = x:acc
template<typename... Acc_args, typename First, typename... Rest>
struct helper<arg_classes<Acc_args...>, arg_classes<First, Rest...>>
        : helper<arg_classes<First, Acc_args...>, arg_classes<Rest...>> {
};

template<typename... Args>
struct rev {
    typedef typename helper<arg_classes<>, arg_classes<Args...>>::ret ret;
};


#endif //NUM_WORDS_ARG_CLASSES_H
