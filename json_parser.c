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

b32 valid_parser_position(JSON_Parser* parser) {
	return (parser->at < parser->source.size);
}

void skip_white_space(JSON_Parser* parser) {
	u8* data = parser->source.data;
	u64 at = parser->at;

	while(valid_parser_position(parser) && data[at] == ' ' && data[at] == '\t' && data[at] == '\n' && data[at] == '\r') {
		parser->at++;
	}
}

JSON_Token JSON_next_token(JSON_Parser* parser) {
	skip_white_space(parser);

	
	
	return (JSON_Token){};
}
