// TODO: Maybe later add the ability to continue parsing even when the error is encountered by
//       assuming that the last token was invalid and finding first valid token. This way, we
//       can give information about more than one error (if there is more than one), instead
//       of just one. This can easily be implemented by just returning error tokens when
//       there is an error, and continuing tokenization from there.

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
} JSON_Parser;

typedef struct {
	JSON_Token token;
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

b32 parser_match_characters(JSON_Parser* parser, s8* expected) {
	while(*expected) {
		if(*expected != parser->source.data[parser->at]) {
			break;
		}
		
		++expected;
		++parser->at;
	}

	return !(*expected);
}

void parser_match_digits(JSON_Parser* parser) {
	while(is_ascii_digit(parser->source.data[parser->at])) {
		++parser->at;
	}
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

			case '-': {
				// NOTE: Maybe change this so that there is no recursive call.
				token.value.data = source.data + parser->at - 1;
				JSON_Token temp_token = JSON_next_token(parser);
				if(temp_token.type == Token_Number) {
					token.value.size = temp_token.value.size + 1;
					token.type = Token_Number;
				}
			} break;

			case '0': {
				token.value.data = source.data + parser->at - 1;
				
				if(parser_match_characters(parser, ".")) {
					parser_match_digits(parser);
					
					token.value.size = (source.data + parser->at) - token.value.data;
					token.type = Token_Number;
				}
			} break;
				
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
				
				parser_match_digits(parser);
				if(parser_match_characters(parser, ".")) {
					parser_match_digits(parser);
				};

				token.value.size = (source.data + parser->at) - token.value.data;
				token.type = Token_Number;
			}
		}
	}
	else {
		token.type = Token_End_Of_Stream;
	}
	
	return token;
}
