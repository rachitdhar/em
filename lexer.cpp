// Lexer.cpp

#include "lexer.h"


// generates tokens for a line
void generate_tokens(Lexer* lexer) {
    int pos = 0;
    bool currently_reading_token = false;
    std::string curr;

    // go through characters in the line
    while (lexer->line[pos]) {
	if (!currently_reading_token) {
	    // skip whitespace
	    while (
	    !lexer->line[pos] ||
	    lexer->line[pos] == ' ' ||
	    lexer->line[pos] == '\t' ||
	    ) pos++;

	    currently_reading_token = true;
	    continue;
	}

	// ways in which a token could end
	//
	// (1) end of line
	// (2) whitespace
	// (3) start of another token type

	char c = lexer->line[pos];

	// starts with number (NUMERIC LITERAL)

	// starts with ' (CHAR LITERAL)

	// starts with " (STRING LITERAL)

	// alpha-numeric + underscores (IDENTIFIER)

	// brackets
	switch (c) {
	case '{':
	    break;
	case '}':
	    break;
	case '(':
	    break;
	case ')':
	    break;
	case '[':
	    break;
	case ']':
	    break;
	}

	pos++;
    }
}

// reads the file line by line and generates tokens
void process_file(Lexer* lexer, const char* file_name) {
    std::ifstream file(file_name);
    if (!file.is_open()) {
	printf("ERROR: Could not find the file");
	exit(1);
    }

    while (std::getline(file, lexer->line)) {
	lexer->line_num++;
	generate_tokens(lexer);
    }

    file.close();
}

int main()
{
    Lexer* lexer = (Lexer*)malloc(sizeof(Lexer));

    process_file(lexer, "program.txt");
    return 0;
}
