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

	for (int i = 0; i < 4; ++i) {
		hash_str += static_cast<char>('!' + (time_char.to_ulong() & 0x3f));
		hash_str += static_cast<char>('!' + (rand_char.to_ulong() & 0x3f));
		time_char >>= 6;
		rand_char >>= 6;
	}
	std::cout << hash_str << std::endl;
}
