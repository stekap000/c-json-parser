#include <stdio.h>

typedef unsigned char u8;

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
} Token_Type;

typedef struct json_token {
	Token_Type type;
	// Value
} JSON_Token;

int main() {
	printf("Init\n");
	
	return 0;
}
