//
// Created by roman on 05.05.17.
//
#include <fstream>
#include <cstring>
#include <iostream>
#include <vector>

const int N = 1 << 15;
const int I = 0;

int main() {
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
    for (int i = 0; i < 8 * 2049; ++i)
        out << "a ";
    out << '\n';
    out.close();
    return 0;
}
