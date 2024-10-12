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

b32 read_and_print_token(JSON_Parser* parser) {
	JSON_Token token = JSON_next_token(parser);
	printf("Type: ");
	print_token_type(token.type);
	printf("\n");

	if(token.type == Token_End_Of_Stream) {
		return 0;
	}

	if(token.type == Token_String || token.type == Token_Number) {
		for(u32 i = 0; i < token.value.size; ++i) {
			printf("%c", token.value.data[i]);
		}
		printf("\n");
	}

	return 1;
}

void print_json_tree_structure(JSON_Node* root, u32 depth) {
	if(depth == 0) {
		printf("root");
	}
	
	while(root) {
		for(u32 i = 0; i < depth; ++i) {
			printf("|   ");
		}
		
		for(u32 i = 0; i < root->label.size; ++i) {
			printf("%c", root->label.data[i]);
		}

		if(root->label.size == 0 && depth != 0) {
			printf("_");
		}

		printf("\n");
		
		if(root->first_child) {
			print_json_tree_structure((JSON_Node*)root->first_child, depth + 1);
		}

		root = (JSON_Node*)root->next_sibling;
	}
}

int main() {
	Buffer json = read_entire_file("small.json");
	JSON_Node* root = JSON_parse(json);
	print_json_tree_structure(root, 0);
	
	return 0;
}
