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

void print_token_type(JSON_Token_Type type) {
	switch(type) {
		case Token_Number:			{ printf("Number"); } break;
		case Token_String:			{ printf("String"); } break;
		case Token_True:			{ printf("True"); } break;
		case Token_False:			{ printf("False"); } break;
		case Token_Null:			{ printf("Null"); } break;
		case Token_Colon:			{ printf("Colon"); } break;
		case Token_Comma:			{ printf("Comma"); } break;
		case Token_Open_Brace:		{ printf("Open_Brace"); } break;
		case Token_Closed_Brace:	{ printf("Closed_Brace"); } break;
		case Token_Open_Bracket:	{ printf("Open_Bracket"); } break;
		case Token_Closed_Bracket:	{ printf("Closed_Bracket"); } break;
		case Token_Error:			{ printf("Error"); } break;
		case Token_End_Of_Stream:	{ printf("End_Of_Stream"); } break;
		case Token_Count:           { printf("Count"); } break;
	}
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

void print_json_tree(JSON_Node* root) {
	JSON_Node* temp = root;

	while(temp) {
		for(u32 i = 0; i < temp->label.size; ++i) {
			printf("%c", temp->label.data[i]);
		}

		if(temp->label.size) {
			printf("\n");
		}

		temp = temp->next_sibling;
		
		if(temp == NULL) {
			temp = root->first_child;
			root = temp;
		}
	}
}

int main() {
#if 0
	Buffer json_buffer = read_entire_file("test.json");
	
	JSON_Parser parser = {json_buffer, 0, 0};
	u32 number_of_tokens_to_read = 20;
	
	for(u32 i = 0; i < number_of_tokens_to_read; ++i) {
		read_and_print_token(&parser);
	}
#else
	//Buffer json_buffer = read_entire_file("net_example.json");
	//JSON_Parser parser = {json_buffer, 0};
	//while(read_and_print_token(&parser));
	Buffer json = read_entire_file("small.json");
	JSON_Node* root = JSON_parse(json);
	print_json_tree(root);
#endif
	
	return 0;
}
