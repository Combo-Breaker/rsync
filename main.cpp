#include "rsync.cpp"
#include "rsync.h"

int main(int argc, char const *argv[]) {
		std::string source(argv[1]), dest(argv[2]);
		rsync(source, dest);
	return 0;
}
