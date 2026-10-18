#include <iostream>
#include "matmul.hpp"

int main() {
    Matrix A(2), B(2);

    A.at(0, 0) = 1;
    A.at(0, 1) = 2;
    A.at(1, 0) = 3;
    A.at(1, 1) = 4;

    B.at(0, 0) = 5;
    B.at(0, 1) = 6;
    B.at(1, 0) = 7;
    B.at(1, 1) = 8;

    Matrix C = matmul_naive(A, B);

    for (std::size_t i = 0; i < C.n; ++i) {
        for (std::size_t j = 0; j < C.n; ++j) {
            std::cout << C.at(i, j) << " ";
        }
        std::cout << std::endl;
    }
}
