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
	Token_Array,
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

void skip_white_space(JSON_Parser* parser) {
	u8* data = parser->source.data;

	while(is_in_bounds(parser->source, parser->at) && is_json_white_space(data[parser->at])) {
		parser->at++;
	}
}

JSON_Token JSON_next_token(JSON_Parser* parser) {
	JSON_Token token = {};
	token.type = Token_Error;
	
	skip_white_space(parser);

	Buffer source = parser->source;

	if(is_in_bounds(source, parser->at)) {
		switch(source.data[parser->at++]) {
			case ':': { token.type = Token_Colon; } break;
			case ',': { token.type = Token_Comma; } break;
			case '{': { token.type = Token_Open_Brace; } break;
			case '}': { token.type = Token_Closed_Brace; } break;
			case '[': { token.type = Token_Open_Bracket; } break;
			case ']': { token.type = Token_Closed_Bracket; } break;

			case '"': {
				token.value.data = source.data + parser->at;

				// TODO: Handle the case where string token will contain \" instead of
				// just ", when there is an escaped quote within the string.
				// Maybe here, or maybe later, when the string value is used.

				// TODO: Check the bound in this loop.

				// Loop until the end of the string and skip over quotes that are escaped.
				// Also handles the case \\", where there is effective no quote escape.
				while(source.data[parser->at] != '"' || (source.data[parser->at - 1] == '\\' && source.data[parser->at - 2] != '\\')) {
					++parser->at;
				}

				token.value.size = (source.data + parser->at) - token.value.data;
				token.type = Token_String;
			} break;
		}
	}
	else {
		fprintf(stderr, "Error accessing source data at invalid index.\n");
	}
	
	return token;
}
