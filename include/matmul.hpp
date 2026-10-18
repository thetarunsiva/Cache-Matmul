#pragma once
#include "matrix.hpp"

inline Matrix matmul_naive(const Matrix& A, const Matrix& B) {
    const std::size_t n = A.n;
    Matrix C(n);

    for (std::size_t i = 0; i < n; ++i) {
        for (std::size_t j = 0; j < n; ++j) {
            for (std::size_t k = 0; k < n; ++k) {
                C.at(i, j) += A.at(i, k) * B.at(k, j);
            }
        }
    }
    return C;
}
