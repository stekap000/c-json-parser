#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

// TODO: Change this to header file after its creation.
#include "json_parser.h"

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
	Buffer json = read_entire_file("test.json");

	JSON_Node* root = JSON_parse(json);
	//print_json_tree_structure(root, 0);

	JSON_Node* pairs = JSON_find_child(root, "pairs");
	// TODO: Make foreach version for these cumbersome iterations.
	for(JSON_Node* coords = (JSON_Node*)pairs->first_child; coords; coords = (JSON_Node*)coords->next_sibling) {
		for(JSON_Node* coord = (JSON_Node*)coords->first_child; coord; coord = (JSON_Node*)coord->next_sibling) {
			f64 number = JSON_node_to_number(coord);
			printf("%.16lf\n", number);
		}
	}
	
	JSON_free(root);
	
	return 0;
}
