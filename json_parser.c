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

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;

typedef char s8;
typedef short s16;
typedef int s32;
typedef long long s64;

typedef float f32;
typedef double f64;

typedef unsigned int b32;

typedef struct buffer {
	u64 size;
	u8* data;
} Buffer;

b32 is_in_bounds(Buffer buffer, u64 index) {
	return (index < buffer.size);
}

typedef enum token_type {
	Token_Number,
	Token_String,
	Token_True,
	Token_False,
	Token_Null,
	Token_Colon,
	Token_Comma,
	Token_Open_Brace,
	Token_Closed_Brace,
	Token_Open_Bracket,
	Token_Closed_Bracket,

	Token_Error,
	Token_End_Of_Stream,
	
	Token_Count
} JSON_Token_Type;

typedef struct json_token {
	JSON_Token_Type type;
	Buffer value;
} JSON_Token;

typedef struct {
	Buffer source;
	u64 at;
	u32 error_encountered;
} JSON_Parser;

typedef struct {
	// TODO: This could change a bit with immediate evaluation.
	Buffer label;
	Buffer value;
	struct JSON_Node* first_child;
	struct JSON_Node* next_sibling;
} JSON_Node;

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

JSON_Node* JSON_parse_node(JSON_Parser* parser) {
	JSON_Node* node = malloc(sizeof(JSON_Node));
	
	JSON_Token token = JSON_next_token(parser);

	if(token.type == Token_Open_Brace) {
		Buffer label = {};
		Buffer value = {};
		while(is_in_bounds(parser->source, parser->at) && !parser->error_encountered) {
			token = JSON_next_token(parser);

			if(token.type == Token_String) {
				label = token.value;

				token = JSON_next_token(parser);

				if(token.type != Token_Comma) {
					parser->error_encountered = 1;
				}
			}
			else {
				parser->error_encountered = 1;
				// TODO: Add some more information about error.
			}

			JSON_Node* subnode = JSON_parse_node(parser);
		}
	}
	else if(token.type == Token_Open_Bracket) {

	}

	return node;
}

JSON_Node* JSON_parse(Buffer json) {
	JSON_Parser parser = {};
	parser.source = json;

	return JSON_parse_node(&parser);
}

