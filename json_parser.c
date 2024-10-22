// TODO: Maybe later add the ability to continue parsing even when the error is encountered by
//       assuming that the last token was invalid and finding first valid token. This way, we
//       can give information about more than one error (if there is more than one), instead
//       of just one. This can easily be implemented by just returning error tokens when
//       there is an error, and continuing tokenization from there.

// TODO: First, parsing will be done with lazy evaluation, so that the value of some node is
//       calculated only when user requests it. Later also add immediate evaluation during parsing
//       just that all nodes have ready value when is is requested.

// TODO: One thing to keep in mind is that tokenization buffers that holds token bytes are just
//       pointers with attached size that point somewhere in json bytes. This leads to undefined
//       behaviour if json bytes somehow become invalid memory. It is faster, but requires this
//       knowledge. Consider adding the option where tokenization buffers are explicitly allocated.

#include <stdlib.h>
#include <stdbool.h>

#include "json_parser.h"

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
			print_json_tree_structure((JSON_Node*)root->first_child, depth + 1);
		}

		root = (JSON_Node*)root->next_sibling;
	}
}
#endif

b32 is_in_bounds(Buffer buffer, u64 index) {
	return (index < buffer.size);
}

// TODO: Maybe change allocation to use arena for whole JSON tree.
JSON_Node* new_json_node() {
	return calloc(1, sizeof(JSON_Node));
}

b32 is_json_white_space(u8 c) {
	return (c == ' ' || c == '\t' || c == '\n' || c == '\r');
}

b32 is_ascii_digit(u8 c) {
	return (c >= '0' && c <= '9');
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
		// TODO: Consider adding pointer to the last child in JSON_Node, so that adding
		//       becomes O(1).
		
		JSON_Node* it = (JSON_Node*)parent->first_child;
		while(it->next_sibling != 0) {
			it = (JSON_Node*)it->next_sibling;
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

	JSON_free_node_subtrees((JSON_Node*)node->first_child);
	JSON_free_node_subtrees((JSON_Node*)node->next_sibling);

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

		JSON_Node* temp = JSON_find((JSON_Node*)current_node->first_child, label);
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

	return JSON_find_sibling((JSON_Node*)node->first_child, label);
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

// TODO: Write custom atof for Buffer type (that would avoid string allocation).
f64 JSON_node_to_number(JSON_Node* node) {
	if(node == 0 || node->value.data == 0 || node->value.size == 0) {
		return 0;
	}

	// This is done to not alter source bytes (keeping them always constant).
	char* string = malloc(node->value.size + 1);
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

	return (node->value.data[0] == 't' &&
			node->value.data[1] == 'r' &&
			node->value.data[2] == 'u' &&
			node->value.data[3] == 'e');
}

// TODO: Handle escaped characters.
char* JSON_node_to_new_string(JSON_Node* node) {
	if(node == 0 || node->value.data == 0 || node->value.size == 0) {
		return 0;
	}
	
	char* string = malloc(node->value.size + 1);
	u64 i;
	for(i = 0; i < node->value.size; ++i) {
		string[i] = node->value.data[i];
	}
	string[i] = 0;
	return string;
}

// TODO: Conversion functions from bytes to different types.
// TODO: Choice of lazy evaluation and upfront (during parsing) evaluation (if possible).

