#include "MathUtil.h"
#include <random>

namespace MathUtil {

	float GetPercent(float number, float percent) {
		return number / 100 * percent;
	}

	int generateRandomInt(int min, int max) {
		std::random_device rd;
		std::mt19937 gen(rd());
		std::uniform_int_distribution<int> dist(min, max);
		return dist(gen);
	}

}