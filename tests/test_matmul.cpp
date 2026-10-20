#include "matmul.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

int checks = 0;

bool close(float actual, float expected) {
    constexpr float absolute_tolerance = 1e-4f;
    constexpr float relative_tolerance = 1e-4f;
    const float difference = std::abs(actual - expected);
    return difference <= absolute_tolerance
        + relative_tolerance * std::max(std::abs(actual), std::abs(expected));
}

void check(bool condition, const std::string& message) {
    ++checks;
    if (!condition) {
        std::cerr << "Test failed: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

void check_matrix(const Matrix& actual, const Matrix& expected,
                  const std::string& message) {
    check(actual.n == expected.n, message + " (dimension)");
    for (std::size_t i = 0; i < actual.data.size(); ++i) {
        check(close(actual.data[i], expected.data[i]), message + " (element)");
    }
}

Matrix identity(std::size_t n) {
    Matrix result(n);
    for (std::size_t i = 0; i < n; ++i) {
        result.at(i, i) = 1.0f;
    }
    return result;
}

Matrix sample(std::size_t n) {
    Matrix result(n);
    for (std::size_t i = 0; i < result.data.size(); ++i) {
        result.data[i] = static_cast<float>(static_cast<int>(i % 11) - 5) / 4.0f;
    }
    return result;
}

template <typename Function>
void check_throws(Function function, const std::string& message) {
    bool threw = false;
    try {
        function();
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    check(threw, message);
}

}  // namespace

int main() {
    Matrix A(2), B(2), expected(2);
    A.data = {1, 2, 3, 4};
    B.data = {5, 6, 7, 8};
    expected.data = {19, 22, 43, 50};

    check_matrix(matmul_naive(A, B), expected, "naive 2x2 known answer");
    check_matrix(matmul_ikj(A, B), expected, "ikj 2x2 known answer");
    check_matrix(matmul_tiled(A, B, 1), expected, "tiled 2x2 known answer");

    for (const std::size_t n : {1U, 3U, 5U, 8U}) {
        const Matrix values = sample(n);
        const Matrix zeros(n);
        const Matrix id = identity(n);

        check_matrix(matmul_naive(values, id), values, "naive identity");
        check_matrix(matmul_ikj(values, id), values, "ikj identity");
        check_matrix(matmul_tiled(values, id, 3), values, "tiled identity");
        check_matrix(matmul_naive(values, zeros), zeros, "naive zero");
        check_matrix(matmul_ikj(values, zeros), zeros, "ikj zero");
        check_matrix(matmul_tiled(values, zeros, 3), zeros, "tiled zero");

        const Matrix reference = matmul_naive(values, values);
        check_matrix(matmul_ikj(values, values), reference, "ikj dimensions");
        check_matrix(matmul_tiled(values, values, 3), reference,
                     "tiled dimensions and partial tiles");
    }

    Matrix different_size(3);
    check_throws([&] { (void)matmul_naive(A, different_size); },
                 "naive dimension mismatch");
    check_throws([&] { (void)matmul_ikj(A, different_size); },
                 "ikj dimension mismatch");
    check_throws([&] { (void)matmul_tiled(A, different_size, 2); },
                 "tiled dimension mismatch");
    check_throws([&] { (void)matmul_tiled(A, B, 0); }, "zero tile size");

    Matrix malformed(2);
    malformed.n = 3;
    check_throws([&] { (void)matmul_naive(malformed, different_size); },
                 "malformed matrix storage");

    check(close(1.0e6f + 50.0f, 1.0e6f), "relative tolerance");
    check(close(0.00005f, 0.0f), "absolute tolerance");

    std::cout << checks << " checks passed\n";
}
