#include "Utility.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <glm/common.hpp>
#include <glm/fwd.hpp>
#include "../third-party/spdlog/include/spdlog/fmt/fmt.h"
#include <fstream>

std::vector<std::byte> Utility::readFile(std::filesystem::path path)
{
	std::ifstream file(path, std::ios::binary | std::ios::ate);
	auto len = file.tellg();
	if (len <= -1) {
		throw std::runtime_error(fmt::format("tellg on file {} failed", path.string()));
	}
	if (len == 0) {
		return std::vector<std::byte>{};
	}
	std::vector<std::byte> buf(len);
	file.seekg(0).read(reinterpret_cast<char *>(buf.data()), len);

	if(file.fail()) {
		throw std::runtime_error(fmt::format("reading file {} failed", path.string()));
	}

	file.close();

	return buf;
}

glm::vec3 Utility::colorFromHsl(float H, float S, float L)
{
	if (H < 0.f) {
		H = 360.f - H;
	}
	if (H > 360.f) {
		H = std::fmod(H, 360);
	}
	S = std::clamp(S, 0.f, 1.f);
	L = std::clamp(L, 0.f, 1.f);

	// https://en.wikipedia.org/wiki/HSL_and_HSV#HSL_to_RGB
	glm::vec3 c;
	float C = (1.f - glm::abs(2 * L - 1)) * S;
	H /= 60.f;
	float X = C * (1.f - glm::abs(std::fmod(H, 2) - 1));

	if(H < 1.f) {
		c = { C, X, 0.f };
	}
	else if (H < 2.f) {
		c = { X, C, 0.f };
	}
	else if (H < 3.f) {
		c = { 0.f, C, X };
	}
	else if (H < 4.f) {
		c = { 0.f, X, C };
	}
	else if (H < 5.f) {
		c = { X, 0.f, C };
	}
	else {
		c = { C, 0.f, X};
	}

	float m = L - C / 2.f;
	return { c.r + m, c.g + m, c.b + m };
}

