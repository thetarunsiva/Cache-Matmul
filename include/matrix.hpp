#pragma once
#include <cstddef>
#include <vector>

struct Matrix {
	std::size_t n;
	std::vector<float> data;

	explicit Matrix(std::size_t size)
		: n(size), data(size * size, 0.0f) {}
	float& at(std::size_t row, std::size_t col) {
		return data[row * n + col];
	}
	const float& at(std::size_t row, std::size_t col) const {
        	return data[row * n + col];
    	}
};
