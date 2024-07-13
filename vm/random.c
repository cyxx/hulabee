
#include <time.h>
#include "intern.h"

static uint32_t _init, _seed;

uint32_t GetRandomNumber(int min, int max) {
	if ((_init & 1) == 0) {
		_init |= 1;
		_seed = time(0);
	}
	if (min != 0) {
		if (min < 0 && min == max) {
			_seed = -min;
			return _seed;
		}
	} else if (max == 0) {
		_seed = time(0);
		return _seed;
	}
	uint32_t seed = _seed * 0x343fd + 0x269ec3;
	_seed = seed;
	seed = (seed >> 16) & 0x7fff;
	return (seed % (max - min + 1)) + min;
}
