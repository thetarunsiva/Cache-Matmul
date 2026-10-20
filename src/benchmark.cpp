#include "matmul.hpp"

#include <algorithm>
#include <charconv>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

using Clock = std::chrono::steady_clock;

volatile double checksum_sink = 0.0;

struct Options {
    std::size_t size = 256;
    std::size_t repetitions = 5;
    std::vector<std::size_t> tiles;
    std::string algorithm = "all";
    bool print_header = true;
    bool profile = false;
};

std::size_t parse_positive(std::string_view text, const char* option) {
    std::size_t value = 0;
    const auto result = std::from_chars(text.data(), text.data() + text.size(), value);
    if (result.ec != std::errc{} || result.ptr != text.data() + text.size()
        || value == 0) {
        throw std::invalid_argument(std::string(option) + " requires a positive integer");
    }
    return value;
}

Options parse_options(int argc, char** argv) {
    Options options;
    for (int i = 1; i < argc; ++i) {
        const std::string_view argument = argv[i];
        const auto next_value = [&](const char* option) -> std::string_view {
            if (++i == argc) {
                throw std::invalid_argument(std::string(option) + " requires a value");
            }
            return argv[i];
        };

        if (argument == "--size") {
            options.size = parse_positive(next_value("--size"), "--size");
        } else if (argument == "--repetitions") {
            options.repetitions = parse_positive(next_value("--repetitions"),
                                                 "--repetitions");
        } else if (argument == "--tile") {
            options.tiles.push_back(parse_positive(next_value("--tile"), "--tile"));
        } else if (argument == "--algorithm") {
            options.algorithm = next_value("--algorithm");
            if (options.algorithm != "all" && options.algorithm != "naive"
                && options.algorithm != "ikj" && options.algorithm != "tiled") {
                throw std::invalid_argument("--algorithm must be all, naive, ikj, or tiled");
            }
        } else if (argument == "--no-header") {
            options.print_header = false;
        } else if (argument == "--profile") {
            options.profile = true;
        } else if (argument == "--help") {
            std::cout << "Usage: matmul_benchmark [--size N] [--repetitions N] "
                         "[--tile N ...] [--algorithm all|naive|ikj|tiled] "
                         "[--no-header] [--profile]\n";
            std::exit(EXIT_SUCCESS);
        } else {
            throw std::invalid_argument("unknown option: " + std::string(argument));
        }
    }
    if (options.tiles.empty()) {
        options.tiles.push_back(32);
    }
    if (options.profile && options.algorithm == "all") {
        throw std::invalid_argument("--profile requires one explicit algorithm");
    }
    return options;
}

Matrix make_input(std::size_t n, std::size_t offset) {
    Matrix matrix(n);
    for (std::size_t i = 0; i < matrix.data.size(); ++i) {
        const std::size_t value = (i * 17 + offset * 29) % 31;
        matrix.data[i] = static_cast<float>(static_cast<int>(value) - 15) / 16.0f;
    }
    return matrix;
}

double checksum(const Matrix& matrix) {
    double result = 0.0;
    for (float value : matrix.data) {
        result += value;
    }
    return result;
}

void verify(const Matrix& actual, const Matrix& expected) {
    constexpr float absolute_tolerance = 1e-3f;
    constexpr float relative_tolerance = 1e-3f;
    for (std::size_t i = 0; i < actual.data.size(); ++i) {
        const float difference = std::abs(actual.data[i] - expected.data[i]);
        const float scale = std::max(std::abs(actual.data[i]), std::abs(expected.data[i]));
        if (difference > absolute_tolerance + relative_tolerance * scale) {
            throw std::runtime_error("correctness check failed at element "
                                     + std::to_string(i));
        }
    }
}

template <typename Function>
void run_benchmark(const std::string& name, std::size_t tile, const Options& options,
                   const Matrix& reference, Function function) {
    Matrix result = function();  // Warm-up is intentionally outside the timed region.
    verify(result, reference);

    std::vector<double> milliseconds;
    milliseconds.reserve(options.repetitions);
    double result_checksum = 0.0;
    for (std::size_t repetition = 0; repetition < options.repetitions; ++repetition) {
        const auto start = Clock::now();
        result = function();
        const auto end = Clock::now();
        milliseconds.push_back(std::chrono::duration<double, std::milli>(end - start).count());
        result_checksum = checksum(result);
        checksum_sink = result_checksum;
    }

    std::sort(milliseconds.begin(), milliseconds.end());
    const std::size_t middle = milliseconds.size() / 2;
    const double median = milliseconds.size() % 2 == 0
        ? (milliseconds[middle - 1] + milliseconds[middle]) / 2.0
        : milliseconds[middle];
    std::cout << name << ',' << options.size << ',' << tile << ','
              << options.repetitions << ',' << std::fixed << std::setprecision(3)
              << median << ',' << std::setprecision(6) << result_checksum << '\n';
}

}  // namespace

int main(int argc, char** argv) {
    try {
        const Options options = parse_options(argc, argv);
        const Matrix A = make_input(options.size, 1);
        const Matrix B = make_input(options.size, 2);

        // Profile mode omits warm-up and reference work so perf sees one kernel.
        // Correctness is established by the normal benchmark path and test suite.
        if (options.profile) {
            Matrix result = options.algorithm == "naive"
                ? matmul_naive(A, B)
                : options.algorithm == "ikj"
                    ? matmul_ikj(A, B)
                    : matmul_tiled(A, B, options.tiles.front());
            checksum_sink = checksum(result);
            std::cout << std::fixed << std::setprecision(6)
                      << "checksum=" << checksum_sink << '\n';
            return EXIT_SUCCESS;
        }

        const Matrix reference = matmul_naive(A, B);

        if (options.print_header) {
            std::cout << "algorithm,size,tile,repetitions,median_ms,checksum\n";
        }
        if (options.algorithm == "all" || options.algorithm == "naive") {
            run_benchmark("naive", 0, options, reference,
                          [&] { return matmul_naive(A, B); });
        }
        if (options.algorithm == "all" || options.algorithm == "ikj") {
            run_benchmark("ikj", 0, options, reference,
                          [&] { return matmul_ikj(A, B); });
        }
        if (options.algorithm == "all" || options.algorithm == "tiled") {
            for (const std::size_t tile : options.tiles) {
                run_benchmark("tiled", tile, options, reference,
                              [&] { return matmul_tiled(A, B, tile); });
            }
        }
    } catch (const std::exception& error) {
        std::cerr << "error: " << error.what() << '\n';
        return EXIT_FAILURE;
    }
}
