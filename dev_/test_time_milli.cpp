#include <bitset>
#include <cstdint>
#include <iostream>
#include <sys/select.h>
#include <sys/time.h>

// int
// main() {
// 	struct timeval time;

// 	gettimeofday(&time, NULL);

// 	std::cout << time.tv_usec << std::endl;
// }

int
main(void) {

	int seed_in = 2;

	struct timeval time;
	gettimeofday(&time, NULL);

	uint32_t t = time.tv_usec;
	std::srand(t);

	uint32_t r = std::rand();

	std::bitset<24> rand_char(r);
	std::bitset<24> time_char(t);

	std::string hash_str;

	for (int i = 0; i < 6; ++i) {
		if (seed_in & 1) {
			hash_str += "abcdefghijklmnopqrstuvwxyz"[(time_char.to_ulong() & 0xf)];
			hash_str += "ABCDEFGHIJKLMNOPQRSTUVWXYZ"[(rand_char.to_ulong() & 0xf)];
		} else {
			hash_str += "ABCDEFGHIJKLMNOPQRSTUVWXYZ"[(time_char.to_ulong() & 0xf)];
			hash_str += "abcdefghijhlmnopqrstuvwxyz"[(rand_char.to_ulong() & 0xf)];
		}
		if (seed_in & 8) {
			hash_str += "0123456789!#$%+-"[(time_char.to_ulong() & 0xf)];
			hash_str += "&'*.^_`|~0123456"[(rand_char.to_ulong() & 0xf)];
		} else {
			hash_str += "&'*.^_`|~0123456"[(time_char.to_ulong() & 0xf)];
			hash_str += "0123456789!#$%+-"[(rand_char.to_ulong() & 0xf)];
		}
		time_char >>= 4;
		rand_char >>= 4;
	}
	std::cout << hash_str << std::endl;
}
