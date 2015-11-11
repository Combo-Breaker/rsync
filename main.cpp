#include <iostream>
#include "rsync.h"

int main(int argc, char const *argv[]) {
	if (argc == 3) {
		rsync(argv[2], argv[3]);

	} else {
		std::cout << "inappropriate usage of rsync";
	}
	return 0;
}
