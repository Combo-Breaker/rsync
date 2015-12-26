#include <iostream>
#include <string.h>
#include "rsync.cpp"
#include "rsync.h"

int main(int argc, char const *argv[]) {
    std::string source(argv[2]), dest(argv[3]);
	if (strcmp(argv[1], "server") == 0) {
		rsync(source, dest, 1);			
	} else if (strcmp(argv[1], "client") == 0) {
		rsync(source, dest, 2);
	}
	return 0;
}
