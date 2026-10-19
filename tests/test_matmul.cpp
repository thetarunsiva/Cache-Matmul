#include <cstdlib>
#include <cmath>
#include <iostream>
#include "matmul.hpp"

bool close(float a, float b) {
    return std::abs(a - b) <= 1e-4f;
}

void check(float actual, float expected) {
    if (!close(actual, expected)) {
        std::cerr << "Test failed: Expected " << expected
                  << ", got " << actual << '\n';
        std::exit(EXIT_FAILURE);
    }
}

int main() {
    Matrix A(2), B(2);

    A.at(0,0) = 1; A.at(0,1) = 2;
    A.at(1,0) = 3; A.at(1,1) = 4;

    B.at(0,0) = 5; B.at(0,1) = 6;
    B.at(1,0) = 7; B.at(1,1) = 8;

    Matrix C = matmul_naive(A, B);

    check(C.at(0,0), 21);
    check(C.at(0,1), 22);
    check(C.at(1,0), 43);
    check(C.at(1,1), 50);

    std::cout << "Naive test passed\n";
}
