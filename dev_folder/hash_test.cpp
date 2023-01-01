#include <bitset>
#include <cstdint>
#include <cstdlib>
#include <string>
#include <sys/types.h>
#include <time.h>

#include <iostream> // need to del

std::string
make_hash_(int& seed_in) {

	std::time_t		t = std::time(0);
	std::bitset<16> left(t);
	std::bitset<8>	right(seed_in);

	if (left[0]) {
		if (left[4]) {
			std::bitset<16> tmp;
			int				i = 15;
			while (i >= 0) {
				tmp[i] = left[15 - i];
				--i;
			}
			left = tmp;
		}
		left.flip();
	}
	if (right[0]) {
		if (right[4]) {
			std::bitset<8> tmp;
			int			   i = 7;
			while (i >= 0) {
				tmp[i] = right[7 - i];
				--i;
			}
			right = tmp;
		}
		right.flip();
	}

	uint32_t rand_seed;
	rand_seed |= left.to_ulong();
	rand_seed <<= 8;
	rand_seed |= right.to_ulong();

	std::srand(rand_seed);
	uint32_t r = std::rand();

	std::bitset<24> total(rand_seed);
	std::string		hash_str;
	int				i = 24;
	while (i >= 0) {
		if (total[i]) {
			hash_str += "abcdefghijklmnopqrstuvwxyz"[i];
		} else {
			hash_str += "ABCDEFGHIJKLMNOPQRSTUVWXYZ"[i];
		}
		if ((1 << i) & r) {
			hash_str += "0123456789"[i % 10];
		} else {
			hash_str += "!@#$%^&*()"[i % 10];
		}
		--i;
	}
	return hash_str;
}

#include <stdlib.h>
int
main(int argc, char const* argv[]) {
	if (argc == 2) {
		int fd = atoi(argv[1]);
		std::cout << make_hash_(fd) << std::endl;
	}

	return 0;
}
