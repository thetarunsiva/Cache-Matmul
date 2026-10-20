#pragma once
#include "matrix.hpp"
#include <algorithm>
#include <stdexcept>

inline void validate_matmul_inputs(const Matrix& A, const Matrix& B) {
    const auto has_valid_storage = [](const Matrix& matrix) {
        return matrix.n == 0
            ? matrix.data.empty()
            : matrix.data.size() / matrix.n == matrix.n
                && matrix.data.size() % matrix.n == 0;
    };

    if (A.n != B.n) {
        throw std::invalid_argument("matrix dimensions must match");
    }
    if (!has_valid_storage(A) || !has_valid_storage(B)) {
        throw std::invalid_argument("matrix storage does not match its dimensions");
    }
}

inline Matrix matmul_naive(const Matrix& A, const Matrix& B) {
    validate_matmul_inputs(A, B);
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

inline Matrix matmul_ikj(const Matrix& A, const Matrix& B) {
    validate_matmul_inputs(A, B);
    const std::size_t n = A.n;
    Matrix C(n);

    for (std::size_t i = 0; i < n; ++i) {
        for (std::size_t k = 0; k < n; ++k) {
            const float a = A.at(i, k);
            for (std::size_t j = 0; j < n; ++j) {
                C.at(i, j) += a * B.at(k, j);
            }
        }
    }
    return C;
}

inline Matrix matmul_tiled(const Matrix& A, const Matrix& B, std::size_t tile) {
    validate_matmul_inputs(A, B);
    if (tile == 0) {
        throw std::invalid_argument("tile size must be greater than zero");
    }
    const std::size_t n = A.n;
    Matrix C(n);

    for (std::size_t ii = 0; ii < n; ii += tile)
        for (std::size_t kk = 0; kk < n; kk += tile)
            for (std::size_t jj = 0; jj < n; jj += tile)
                for (std::size_t i = ii; i < std::min(ii + tile, n); ++i)
                    for (std::size_t k = kk; k < std::min(kk + tile, n); ++k) {
                        const float a = A.at(i, k);
                        for (std::size_t j = jj;
                             j < std::min(jj + tile, n); ++j)
                            C.at(i, j) += a * B.at(k, j);
                    }
    return C;
}
