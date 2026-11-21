#include <iostream>

int main(int argc, char** argv) {
    int mode = argc > 1 ? argv[1][0] - '0' : 0;
    int* p = new int{42};
    if (mode == 0) return 0;
    delete p;
    if (mode == 1) std::cout << *p << '\n';
    if (mode == 2) delete p;
}