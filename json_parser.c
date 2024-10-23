// TODO(stekap) [+++]: Maybe arena allocator.
// TODO(stekap) [+  ]: Maybe separate source buffer.
// TODO(stekap) [+  ]: Maybe add choice of upfront evaluation.
// TODO(stekap) [-  ]: Maybe add label string escape evaluation.

#include <stdlib.h>
#include <stdbool.h>

#include "json_parser.h"

#define MAX_BYTE_VALUE 255

// TODO(stekap): Remove later.
#include <assert.h>
#define NOT_YET_IMPLEMENTED(msg) assert(!msg)

#ifdef JSON_PARSER_DEBUG
#include <stdio.h>

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
			print_json_tree_structure(root->first_child, depth + 1);
		}

		root = root->next_sibling;
	}
}
#endif

b32 is_in_bounds(Buffer buffer, u64 index) {
	return (index < buffer.size);
}

JSON_Node* new_json_node() {
	return JSON_CALLOC(1, sizeof(JSON_Node));
}

b32 is_json_white_space(u8 c) {
	return (c == ' ' || c == '\t' || c == '\n' || c == '\r');
}

b32 is_ascii_digit(u8 c) {
	return (c >= '0' && c <= '9');
}

b32 is_ascii_hex_digit(u8 c) {
	return ((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'));
}

u8 ascii_hex_digit_to_byte(u8 c) {
	if(c >= '0' && c <= '9') {
		return c - '0';
	}
	else if(c >= 'a' && c <= 'f') {
		return c - 'a' + 10;
	}
	else if(c >= 'A' && c <= 'F') {
		return c - 'A' + 10;
	}

	return MAX_BYTE_VALUE;
}

void parser_skip_white_space(JSON_Parser* parser) {
	u8* data = parser->source.data;

	while(is_in_bounds(parser->source, parser->at) && is_json_white_space(data[parser->at])) {
		parser->at++;
	}
}

b32 parser_match_character(JSON_Parser* parser, s8 c) {
	if(is_in_bounds(parser->source, parser->at) && parser->source.data[parser->at] == c) {
		++parser->at;
		return 1;
	}

	return 0;
}

b32 parser_look_character(JSON_Parser* parser, s8 c) {
	if(is_in_bounds(parser->source, parser->at) && parser->source.data[parser->at] == c) {
		return 1;
	}

	return 0;
}

b32 parser_look_back_character(JSON_Parser* parser, s8 c) {
	if(parser->source.data[parser->at - 1] == c) {
		return 1;
	}

	return 0;
}

b32 parser_match_characters(JSON_Parser* parser, s8* expected) {
	while(*expected) {
		if(is_in_bounds(parser->source, parser->at) && *expected != parser->source.data[parser->at]) {
			break;
		}
		
		++expected;
		++parser->at;
	}

	return !(*expected);
}

b32 parser_match_digits(JSON_Parser* parser) {
	b32 at_least_one_digit_found = 0;
	while(is_ascii_digit(parser->source.data[parser->at])) {
		++parser->at;
		++at_least_one_digit_found;
	}
	return at_least_one_digit_found;
}

JSON_Token JSON_next_token(JSON_Parser* parser) {
	JSON_Token token = {};
	token.type = Token_Error;
	
	parser_skip_white_space(parser);

	Buffer source = parser->source;
	
	if(is_in_bounds(source, parser->at)) {
		switch(source.data[parser->at++]) {
			case ':': { token.type = Token_Colon; } break;
			case ',': { token.type = Token_Comma; } break;
			case '{': { token.type = Token_Open_Brace; } break;
			case '}': { token.type = Token_Closed_Brace; } break;
			case '[': { token.type = Token_Open_Bracket; } break;
			case ']': { token.type = Token_Closed_Bracket; } break;

			case 't': { if(parser_match_characters(parser, "rue")) token.type = Token_True; } break;
			case 'f': { if(parser_match_characters(parser, "alse")) token.type = Token_False; } break;
			case 'n': { if(parser_match_characters(parser, "ull")) token.type = Token_Null; } break;
				
			case '"': {
				token.value.data = source.data + parser->at;

				// At this point, we leave \" and other special characters without
				// converting them to their byte values.
				
				// Loops until the end of the string and skips over quotes that are escaped.
				// Also handles the case \\", where there is effectively no quote escape.
				JSON_Token_Type string_or_error_type = Token_Error;
				while(1) {
					if(!is_in_bounds(source, parser->at)) {
						break;
					}

					if(source.data[parser->at] == '"') {
						if(!is_in_bounds(source, parser->at - 1)) {
							break;
						}
						
						if(source.data[parser->at - 1] != '\\') {
							string_or_error_type = Token_String;
							break;
						}

						if(!is_in_bounds(source, parser->at - 2)) {
							break;
						}
						
						if(source.data[parser->at - 2] == '\\') {
							string_or_error_type = Token_String;
							break;
						}
					}

					++parser->at;
				}

				token.value.size = (source.data + parser->at) - token.value.data;
				token.type = string_or_error_type;

				// Handles the case where break happens and increment in loop is skipped.
				++parser->at;
			} break;

			case '-':
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9': {
				// -1 in order to also include matched case digit into the final number.
				token.value.data = source.data + parser->at - 1;

				// If it is not '-0' or '0', then try to match digits.
				if(!(parser_look_back_character(parser, '-') && parser_look_character(parser, '0')) && !parser_look_back_character(parser, '0')) {
					parser_match_digits(parser);
				}

				// This handles the case where we have '-0' or something like '000000'.
				if(!parser_look_back_character(parser, '0')) {
					parser_match_character(parser, '0');
				}

				if(parser_match_character(parser, '.')) {
					// If there are no digits at all after '.', then we go back one place, thus tokenizing
					// what is before '.'.
					if(!parser_match_digits(parser)) {
						--parser->at;
					}
				}
				
				if(parser_match_character(parser, 'e') || parser_match_character(parser, 'E')) {
					if(parser_look_character(parser, '+') || parser_look_character(parser, '-')) {
						++parser->at;
					}

					// If no digits are matched after 'e' or 'e[+/-]', then move back to the number
					// before 'e'.
					if(!parser_match_digits(parser)) {
						--parser->at;
						if(parser_look_character(parser, '+') || parser_look_character(parser, '-')) {
							--parser->at;
						}
					}
				}

				// If we didn't just match '-', but a valid number with/without '-'.
				if(!parser_look_back_character(parser, '-')) {
					token.value.size = (source.data + parser->at) - token.value.data;
					token.type = Token_Number;
				}
			}
		}
	}
	else {
		token.type = Token_End_Of_Stream;
	}
	
	return token;
}

void attach_child_node(JSON_Node* parent, JSON_Node* child) {
	if(parent->first_child) {
		// TODO(stekap): Consider adding pointer to the last child in JSON_Node, so that adding
		//               becomes O(1).
		
		JSON_Node* it = parent->first_child;
		while(it->next_sibling != 0) {
			it = it->next_sibling;
		}
		it->next_sibling = (struct JSON_Node*)child;
	}
	else {
		parent->first_child = (struct JSON_Node*)child;
	}
}

void JSON_parse_node(JSON_Parser* parser, JSON_Node* parent_node) {
	JSON_Token token = JSON_next_token(parser);

	switch(token.type) {
		case Token_Open_Brace: {
			while(is_in_bounds(parser->source, parser->at) && !parser->error_encountered) {
				Buffer label = {};
				token = JSON_next_token(parser);

				if(token.type == Token_String) {
					label = token.value;

					token = JSON_next_token(parser);

					if(token.type != Token_Colon) {
						parser->error_encountered = 1;
						break;
					}
				}
				else if(token.type == Token_Closed_Brace) {
					break;
				}
				else {
					parser->error_encountered = 1;
				}

				JSON_Node* node = new_json_node();
				node->label = label;
				attach_child_node(parent_node, node);

				JSON_parse_node(parser, node);

				token = JSON_next_token(parser);

				if(token.type == Token_Closed_Brace) {
					break;
				}
				else if(token.type != Token_Comma) {
					parser->error_encountered = 1;
				}
			}
		} break;

		case Token_Open_Bracket: {
			if(parser->source.data[parser->at] == ']') {
			 	JSON_next_token(parser);
			 	break;
			}
			
			while(is_in_bounds(parser->source, parser->at) && !parser->error_encountered) {
				JSON_Node* node = new_json_node();
				attach_child_node(parent_node, node);

				JSON_parse_node(parser, node);

				token = JSON_next_token(parser);

				if(token.type == Token_Closed_Bracket) {
					break;
				}
				else if(token.type != Token_Comma) {
					parser->error_encountered = 1;
				}
			}
		} break;

		case Token_Number:
		case Token_String:
		case Token_True:
		case Token_False:
		case Token_Null: {
			if(parent_node) {
				parent_node->value = token.value;
			}
		} break;

		case Token_End_Of_Stream: {} break;

		default: {
			parser->error_encountered = 1;
		}
	}
}

JSON_Node* JSON_parse(u8* data, u64 size) {
	JSON_Parser parser = {};
	parser.source.data = data;
	parser.source.size = size;

	JSON_Node* root = new_json_node();
	JSON_parse_node(&parser, root);

	// Indicate failure with null pointer.
	if(parser.error_encountered) {
		JSON_free(root);
		return 0;
	}

	return root;
}

void JSON_free_node_subtrees(JSON_Node* node) {
	if(node == 0) {
		return;
	}

	JSON_free_node_subtrees(node->first_child);
	JSON_free_node_subtrees(node->next_sibling);

	free(node->first_child);
	free(node->next_sibling);
}

// NOTE: This version that relies on first freeing subtrees and then root node
//       feels better to use than version where we would have only one function
//       that would need to take address of a pointer to root node.
void JSON_free(JSON_Node* node) {
	JSON_free_node_subtrees(node);
	free(node);
}

b32 buffer_and_string_are_equal(Buffer buffer, char* string) {
	u64 i;
	for(i = 0; i < buffer.size; ++i) {
		if(string[i] == 0 || buffer.data[i] != string[i]) {
			return 0;
		}
	}

	if(string[i] == 0) {
		return 1;
	}
	
	return 0;
}

JSON_Node* JSON_find(JSON_Node* node, char* label) {
	if(node == 0) {
		return 0;
	}
	
	foreach_sibling(node, current_node) {
		if(buffer_and_string_are_equal(current_node->label, label)) {
			return current_node;
		}

		JSON_Node* temp = JSON_find(current_node->first_child, label);
		if(temp) {
			return temp;
		}
	}

	return 0;
}

JSON_Node* JSON_find_sibling(JSON_Node* node, char* label) {
	if(node == 0) {
		return 0;
	}
	
	foreach_sibling(node, current_node) {
		if(buffer_and_string_are_equal(current_node->label, label)) {
			return current_node;
		}
	}

	return 0;
}

JSON_Node* JSON_find_child(JSON_Node* node, char* label) {
	if(node == 0) {
		return 0;
	}

	return JSON_find_sibling(node->first_child, label);
}

bool JSON_node_is_null(JSON_Node* node) {
	if(node == 0 || node->value.data == 0 || node->value.size != 4) {
		return 0;
	}

	return (node->value.data[0] == 'n' &&
			node->value.data[1] == 'u' &&
			node->value.data[2] == 'l' &&
			node->value.data[3] == 'l');
}

// TODO(stekap): Write custom atof for Buffer type (that would avoid string allocation).
f64 JSON_node_to_number(JSON_Node* node) {
	if(node == 0 || node->value.data == 0 || node->value.size == 0) {
		return 0;
	}

	// This is done to not alter source bytes (keeping them always constant).
	char* string = JSON_MALLOC(node->value.size + 1);
	for(u64 i = 0; i < node->value.size; ++i) {
		string[i] = node->value.data[i];
	}
	string[node->value.size] = 0;
	f64 number = atof(string);
	free(string);

	return number;
}

char JSON_node_to_char(JSON_Node* node) {
	if(node == 0 || node->value.data == 0 || node->value.size == 0) {
		return 0;
	}
	
	return node->value.data[0];
}

bool JSON_node_to_bool(JSON_Node* node) {
	if(node == 0 || node->value.data == 0 || node->value.size < 4) {
		return 0;
	}

	return (node->value.size    ==  4  &&
			node->value.data[0] == 't' &&
			node->value.data[1] == 'r' &&
			node->value.data[2] == 'u' &&
			node->value.data[3] == 'e');
}

char* JSON_node_to_new_string(JSON_Node* node) {
	if(node == 0 || node->value.data == 0 || node->value.size == 0) {
		return 0;
	}
	
	char* string = JSON_MALLOC(node->value.size + 1);
	u64 i;
	for(i = 0; i < node->value.size; ++i) {
		string[i] = node->value.data[i];
	}
	string[i] = 0;
	return string;
}

char* JSON_node_to_new_string_resolved(JSON_Node* node) {
	if(node == 0 || node->value.data == 0 || node->value.size == 0) {
		return 0;
	}

	char* string = JSON_MALLOC(node->value.size + 1);
	
	u64 buffer_index = 0;
	u64 string_index = 0;
	while(buffer_index < node->value.size) {
		if(node->value.data[buffer_index] == '\\') {
			// '\' can't be the last string character since parser indicates error in that case.
			switch(node->value.data[++buffer_index]) {
				case '\\': { string[string_index++] = '\\'; } break;
				case '/' : { string[string_index++] = '/';  } break;
				case 'b' : { string[string_index++] = '\b'; } break;
				case 'f' : { string[string_index++] = '\f'; } break;
				case 'n' : { string[string_index++] = '\n'; } break;
				case 'r' : { string[string_index++] = '\r'; } break;
				case 't' : { string[string_index++] = '\t'; } break;
				case 'u' : {
					// All 4 hex digits must follow.
					if((buffer_index+4) < node->value.size) {
						u8 b1 = ascii_hex_digit_to_byte(node->value.data[buffer_index+1]);
						u8 b2 = ascii_hex_digit_to_byte(node->value.data[buffer_index+2]);
						u8 b3 = ascii_hex_digit_to_byte(node->value.data[buffer_index+3]);
						u8 b4 = ascii_hex_digit_to_byte(node->value.data[buffer_index+4]);
						
						if(b1 == MAX_BYTE_VALUE || b2 == MAX_BYTE_VALUE || b3 == MAX_BYTE_VALUE || b4 == MAX_BYTE_VALUE) {
							free(string);
							return 0;
						}

						string[string_index++] = (u8)((b3 << 4) | b4);
						string[string_index++] = (u8)((b1 << 4) | b2);

						buffer_index += 4;
					}
					else {
						free(string);
						return 0;
					}
				} break;
				default: {
					free(string);
					return 0;
				} break;
			}

			++buffer_index;
		}
		else {
			string[string_index++] = node->value.data[buffer_index++];
		}
	}

	string[string_index] = 0;

	if(string_index != buffer_index) {
		string = JSON_REALLOC(string, string_index + 1);
	}

	return string;
}

