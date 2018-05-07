#include "class.h"
#include <endian.h>
#include <errno.h>
#include "print.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *args[]) {
	if (argc == 1) {
		printf("Please pass at least 1 .class file to open");
		exit(EXIT_FAILURE);
	}

	int i;
	for (i = 1; i < argc; i++) {
		char *file_name = args[i];
		FILE *file = fopen(file_name, "r");
		fseek(file, 0, SEEK_END);
		long len = ftell(file);
		char *data = malloc(len);
		fseek(file, 0, SEEK_SET);
		fread(data, 1, len, file);
		if (!file) {
			printf("Could not open '%s': %s\n", file_name, strerror(errno));
			continue;
		}
		Bytecode bytecode = {
				.data = data,
						.length = len,
								.index = 0,
		};

		Class *class = read_class(bytecode);
		if (class == NULL) {
			fprintf(stderr, "Parsing aborted; invalid class file contents: %s\n", class_file.file_name);
		} else {
			// yay, valid!
			print_class(file, class);
		}

		free(class);
		fclose(file);
	}

	exit(EXIT_SUCCESS);
}

