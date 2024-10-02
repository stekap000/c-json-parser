#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

// TODO: Change this to header file after its creation.
#include "json_parser.c"

Buffer read_entire_file(char* filename) {
	Buffer buffer = {};

	FILE* file = fopen(filename, "rb");
	if(!file) {
		fprintf(stderr, "Error opening file.\n");
		return buffer;
	}

	fseek(file, 0, SEEK_END);
	buffer.size = ftell(file);
	buffer.data = malloc(buffer.size);
	fseek(file, 0, SEEK_SET);

	if(fread(buffer.data, 1, buffer.size, file) != buffer.size) {
		fprintf(stderr, "Error reading file.\n");
		fclose(file);
		return buffer;
	}

	fclose(file);
	return buffer;
}

int main() {
	Buffer json_buffer = read_entire_file("test.json");
	
	JSON_Parser parser = {json_buffer, 0};
	JSON_Token token = JSON_next_token(&parser);
	printf("Type: %d\n", token.type);
	token = JSON_next_token(&parser);
	printf("Type: %d\n", token.type);
	for(int i = 0; i < token.value.size; ++i) {
		printf("%c", token.value.data[i]);
	}
	
	
	return 0;
}
