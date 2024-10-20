#ifndef JSON_PARSER_H
#define JSON_PARSER_H

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

#define foreach_child(node, iterator) for(JSON_Node* iterator = (JSON_Node*)node->first_child; iterator; iterator = (JSON_Node*)iterator->next_sibling)

#define foreach_sibling(node, iterator) for(JSON_Node* iterator = (JSON_Node*)node; iterator; iterator = (JSON_Node*)iterator->next_sibling)

void print_token_type(JSON_Token_Type type);
void print_json_tree_structure(JSON_Node* root, u32 depth);

JSON_Node* JSON_parse(u8* data, u64 size);
void JSON_free_node_subtrees(JSON_Node* node);
void JSON_free(JSON_Node* node);
JSON_Node* JSON_find(JSON_Node* node, char* label);
JSON_Node* JSON_find_sibling(JSON_Node* node, char* label);
JSON_Node* JSON_find_child(JSON_Node* node, char* label);
f64 JSON_node_to_number(JSON_Node* node);

#endif // JSON_PARSER_H
