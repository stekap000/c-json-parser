typedef unsigned char u8;
typedef unsigned long long u64;

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

JSON_Token JSON_next_token() {
	return (JSON_Token){};
}
