//
// Created by roman on 03.07.17.
//

#include "trampoline.h"

int main(int argc, char const *argv[]) {
    printf("Test variable capture\n");
    int a = 0;
    trampoline<int(int, int)> c2([&](int x, int y) {
        printf("a = %d, x = %d, y = %d\n", a, x, y);
        a = 2;
        return y;
    });
    auto ptr = c2.get();
    ptr(2, 1);
    std::cout << "a = " << a << '\n';
    a = 1;
    ptr(2, 1);


    trampoline<int()> t0([&]() { return printf("\n\nArgumentless lambda\na = %d\n", a); });
    auto ptr0 = t0.get();
    ptr0();
    auto lam = [&](int u, int v, double d, int x, std::string &s) {
        std::cout << u << ' ' << v << ' ' << x << ' ' << d << ' ' << s << '\n';
        return u;
    };


    trampoline<int(int, int, double, int, std::string &)> t5(lam);
    auto ptr5 = t5.get();
    double d = 3.14;
    printf("%lf\n", d);
    std::string s = "abcaabcaabcaabcaabcaabcaabcaabca";
    std::cout << ptr5(2, 3, d, 4, s) << '\n';


    auto lam_many_ints = [](int i1, int i2, int i3, int i4, int i5, int i6, int i7) {
        printf("\n\nMore than 6 ints\n");
        printf("%d %d %d %d %d %d %d \n", i1, i2, i3, i4, i5, i6, i7);
    };
    trampoline<void(int, int, int, int, int, int, int)> tr_many_ints(lam_many_ints);
    tr_many_ints.get()(1, 2, 3, 4, 5, 6, 7);


    auto lam_fun = [](int i1, int i2, int i3, int i4, int i5, float f1, float f2, float f3, int i6, float f4, float f5,
                      float f6, float f7, float f8, float f9, float f10, int i7) {
        printf("\n\nMore than 6 ints, more than 8 SSEs\n");
        printf("%d %d %d %d %d %f %f %f %d %f %f %f %f %f %f %f %d\n", i1, i2, i3, i4, i5, f1, f2, f3, i6, f4, f5, f6,
               f7, f8,
               f9, f10, i7);
    };
    trampoline<void(int, int, int, int, int, float, float, float, int, float, float, float, float, float, float, float,
                    int)> tr_fun(
            lam_fun
    );
    tr_fun.get()(1, 2, 3, 4, 5, 6.f, 7.f, 8.f, 25, 9.f, 10.f, 11.f, 12.f, 13.f, 14.f, 15.f, 16);


    auto lam_gun = [](int i1, int i2, int i3, int i4, int i5, float f1, float f2, float f3, float f4, float f5,
                      float f6, float f7, float f8, float f9, float f10, int i6) {
        printf("\n\nMore than 8 SSEs before 6th int\n");
        printf("%d %d %d %d %d %f %f %f %f %f %f %f %f %f %f %d\n", i1, i2, i3, i4, i5, f1, f2, f3, f4, f5, f6, f7, f8,
               f9, f10, i6);
    };
    lam_gun(1, 2, 3, 4, 5, 6.f, 7.f, 8.f, 9.f, 10.f, 11.f, 12.f, 13.f, 14.f, 15.f, 16);
    trampoline<void(int, int, int, int, int, float, float, float, float, float, float, float, float, float, float,
                    int)> tr_gun(
            lam_gun
    );
    tr_gun.get()(1, 2, 3, 4, 5, 6.f, 7.f, 8.f, 9.f, 10.f, 11.f, 12.f, 13.f, 14.f, 15.f, 16);


    typedef std::string st;
    auto lam6 = [&](st *s0, st *s1, st *s2, st *s3, double d0, double d1, double d2, double d3, double d4, double d5, double d6, double d7,
                    st *s4, st *s5, float x, double z) {
        printf("%s %s %s %s %lf %lf %lf %lf %lf %lf %lf %lf %s %s %f %lf\n", s0->c_str(), s1->c_str(), s2->c_str(), s3->c_str(), d0, d1, d2, d3, d4, d5, d6, d7,
               s4->c_str(), s5->c_str(), x, z);
        return x;
    };
    trampoline<double(st *, st *, st *, st *, double, double, double, double, double, double, double, double, st *,
                      st *, float, double)> tr6(
            lam6
    );
    st s0 = "aaa";
    st s1 = "bbb";
    st s2 = "ccc";
    st s3 = "ddd";
    st s4 = "eee";
    st s5 = "fff";
    tr6.get()(&s0, &s1, &s2, &s3, 0., 1., 2., 3., 4., 5., 6., 7., &s4, &s5, 1.23f, 2.34);


    printf("\n\nTest copying\n");
    trampoline<double(st *, st *, st *, st *, double, double, double, double, double, double, double, double, st *,
                      st *, float, double)> tr_copy(tr6);
    tr_copy.get()(&s0, &s1, &s2, &s3, 0., 1., 2., 3., 4., 5., 6., 7., &s4, &s5, 1.23f, 2.34);
    return 0;
}