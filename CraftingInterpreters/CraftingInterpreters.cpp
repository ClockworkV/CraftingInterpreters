// CraftingInterpreters.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <string>
#include <string_view>
#include <filesystem>
#include <fstream>
#include <optional>
#include <variant>
#include <functional>

enum class TokenType {
	// Single-character tokens.                      
	LEFT_PAREN, RIGHT_PAREN, LEFT_BRACE, RIGHT_BRACE,
	COMMA, DOT, MINUS, PLUS, SEMICOLON, SLASH, STAR,

	// One or two character tokens.                  
	BANG, BANG_EQUAL,
	EQUAL, EQUAL_EQUAL,
	GREATER, GREATER_EQUAL,
	LESS, LESS_EQUAL,

	// Literals.                                     
	IDENTIFIER, STRING, NUMBER,

	// Keywords.                                     
	AND, CLASS, ELSE, FALSE, FUN, FOR, IF, NIL, OR,
	PRINT, RETURN, SUPER, THIS, TRUE, VAR, WHILE,

	EOF_LOX
};

const char* TokenNames[] = {
	"LEFT_PAREN", "RIGHT_PAREN", "LEFT_BRACE", "RIGHT_BRACE",
	"COMMA", "DOT", "MINUS", "PLUS", "SEMICOLON", "SLASH", "STAR",

	// One or two character tokens.                  
	"BANG", "BANG_EQUAL",
	"EQUAL", "EQUAL_EQUAL",
	"GREATER", "GREATER_EQUAL",
	"LESS", "LESS_EQUAL",

	// Literals.                                     
	"IDENTIFIER", "STRING", "NUMBER",

	// Keywords.                                     
	"AND", "CLASS", "ELSE", "FALSE", "FUN", "FOR", "IF", "NIL", "OR",
	"PRINT", "RETURN", "SUPER", "THIS", "TRUE", "VAR", "WHILE",

	"EOF_LOX"
};

class Token
{
public:
	using Literal = std::optional<std::variant<double, std::string>>;
	Token() = default;
	Token(TokenType _type, std::string_view _lexeme, Literal _literal, int _line):
		type(_type), lexeme(_lexeme), literal(_literal), line(_line)
	{

	}

	std::string getLiteral() const
	{
		if (!literal.has_value())
			return std::string();
		if ((*literal).index() == 0)
			return std::to_string(std::get<double>(*literal));
		else
			return std::get<std::string>(*literal);
	}
	std::string toString() const
	{
		return std::string(TokenNames[int(type)]) + " " + std::string(lexeme) + " " + getLiteral();

	}
private:
	TokenType type;
	std::string_view lexeme;
	Literal literal;
	int line;
};


class Scanner
{
public:
	std::vector<Token> scanTokens() {
		while (!isAtEnd()) {
			// We are at the beginning of the next lexeme.
			start = current;
			scanToken();
		}

		tokens.emplace_back(TokenType::EOF_LOX,
			std::string_view(""),
			std::nullopt,
			line);
		return tokens;
	}
	Scanner(std::string s, std::function<void(int, std::string_view)> err_handler ) 
		:source(s), error_handler(err_handler) {}

private:
	bool isAtEnd()
	{
		return current >= int(source.length());
	}

	void scanToken() {
		char c = advance();
		switch (c) {
		case '(': addToken(TokenType::LEFT_PAREN); break;
		case ')': addToken(TokenType::RIGHT_PAREN); break;
		case '{': addToken(TokenType::LEFT_BRACE); break;
		case '}': addToken(TokenType::RIGHT_BRACE); break;
		case ',': addToken(TokenType::COMMA); break;
		case '.': addToken(TokenType::DOT); break;
		case '-': addToken(TokenType::MINUS); break;
		case '+': addToken(TokenType::PLUS); break;
		case ';': addToken(TokenType::SEMICOLON); break;
		case '*': addToken(TokenType::STAR); break;
		case '!': addToken(match('=') ? TokenType::BANG_EQUAL : TokenType::BANG); break;
		case '=': addToken(match('=') ? TokenType::EQUAL_EQUAL : TokenType::EQUAL); break;
		case '<': addToken(match('=') ? TokenType::LESS_EQUAL : TokenType::LESS); break;
		case '>': addToken(match('=') ? TokenType::GREATER_EQUAL : TokenType::GREATER); break;
		case '/':
			if (match('/')) {
				// A comment goes until the end of the line.                
				while (peek() != '\n' && !isAtEnd()) advance();
			}
			else {
				addToken(TokenType::SLASH);
			}
			break;
		case ' ':
		case '\r':
		case '\t':
			// Ignore whitespace.                      
			break;

		case '\n':
			line++;
			break;
		
		case '"': scanString(); break;

		default:
			if (isDigit(c)) {
				scanNumber();
			}
			else if (isAlpha(c)) {
				scanIdentifier();
			}
			else {
				error_handler(line, "Unexpected character");
			}
		}
	}
	void scanIdentifier() {
		while (isAlphaNumeric(peek())) advance();

		std::string text = source.substr(start, current);

		TokenType type = TokenType::IDENTIFIER;
		auto keyword = keywords.find(text);
		if (keyword != keywords.end()) type = keyword->second;
		addToken(type);
	}
	void scanNumber() {
		while (isDigit(peek())) advance();

		// Look for a fractional part.                            
		if (peek() == '.' && isDigit(peekNext())) {
			// Consume the "."                                      
			advance();

			while (isDigit(peek())) advance();
		}

		addToken(TokenType::NUMBER,
			std::stod(std::string(source.data() + start, current - start)));
	}
	void scanString() {
		while (peek() != '"' && !isAtEnd()) {
			if (peek() == '\n') line++;
			advance();
		}

		// Unterminated string.                                 
		if (isAtEnd()) {
			error_handler(line, "Unterminated string.");
			return;
		}

		// The closing ".                                       
		advance();

		// Trim the surrounding quotes.                         
		std::string text(source.data() + start, current - start);
		addToken(TokenType::STRING, text);
	}
	bool match(char expected) {
		if (isAtEnd()) return false;
		if (source[current] != expected) return false;

		current++;
		return true;
	}
	char peek() {
		if (isAtEnd()) return '\0';
		return source[current];
	}
	char peekNext() {
		if (current + 1 >= source.length()) return '\0';
		return source[current + 1];
	}
	bool isAlpha(char c) {
		return (c >= 'a' && c <= 'z') ||
			(c >= 'A' && c <= 'Z') ||
			c == '_';
	}
	bool isAlphaNumeric(char c) {
		return isAlpha(c) || isDigit(c);
	}
	bool isDigit(char c) {
		return c >= '0' && c <= '9';
	}
	char advance() {
		current++;
		return source[current - 1];
	}

	void addToken(TokenType type) {
		addToken(type, Token::Literal{});
	}

	void addToken(TokenType type, Token::Literal literal) {
		std::string_view text = std::string_view(source.data() + start, current - start);
		tokens.emplace_back(Token(type, text, literal, line));
	}
	std::string source;
	std::vector<Token> tokens;
	std::function<void(int, std::string_view)> error_handler;
	int start = 0;
	int current = 0;
	int line = 1;
	static std::unordered_map<std::string, TokenType> keywords;

};

std::unordered_map<std::string, TokenType> Scanner::keywords{
		{"and", TokenType::AND},
		{"class", TokenType::CLASS},
		{"else", TokenType::ELSE},
		{"false", TokenType::FALSE},
		{"for", TokenType::FOR},
		{"fun", TokenType::FUN},
		{"if", TokenType::IF},
		{"nil", TokenType::NIL},
		{"or", TokenType::OR},
		{"print", TokenType::PRINT},
		{"return", TokenType::RETURN},
		{"super", TokenType::SUPER},
		{"this", TokenType::THIS},
		{"true", TokenType::TRUE},
		{"var", TokenType::VAR},
		{"while", TokenType::WHILE}
};

class Lox
{
	using string = std::string;
	using string_view = std::string_view;
public:
	void main(int argc, char* argv[])
	{
		if (argc > 2) {
			std::cout << "Usage: jlox [script]\n" ;
			exit(64);
		}

		else if (argc == 2) {
			runFile(argv[1]);
		}
		else {
			runPrompt();
		}
	}

	void runFile(std::string_view path)
	{
		std::fstream fs(path.data());
		std::string program;
		fs >> program;
		run(program);
		if (hadError) exit(65);
	}

	void runPrompt()
	{
		for(;;)
		{
			std::cout << "> ";
			string program;
			std::getline(std::cin, program);
			run(program);
			hadError = false;
		}

	}

	void run(const std::string& program)
	{
		Scanner scanner(program, [this](int line, std::string_view message) {error(line, message); });
		auto tokens = scanner.scanTokens();

		for (const auto& token: tokens)
		{
			std::cout << token.toString() << '\n';
		}
	}

	void error(int line, string_view message)
	{
		report(line, "", message);
	}

private:
	void report(int line, string_view where, string_view message)
	{
		std::cout << "[line " << line << "] Error" << where << ": " << message << '\n';
		hadError = true;
	}

	bool hadError = false;
};

int main(int argc, char* argv[])
{
	Lox lox;
	lox.main(argc, argv);
	return 0;
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
