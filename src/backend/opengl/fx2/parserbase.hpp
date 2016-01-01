// Reusable parser base
#pragma once
#include <istream>
#include <string>
#include <vector>
#include <memory>
#include <fstream>
#include <iostream>
#include <sstream>
#include <cctype>
#include <cstring>
#include <filesystem/path.h>

namespace parse
{
	namespace {
		std::string loadSource(const char* path)
		{
			std::ifstream fileIn(path, std::ios::in);
			if (!fileIn.is_open()) {
                std::cerr << "Could not open file " << path << std::endl;
				throw std::runtime_error("Could not open file");
			}
			std::string str;
			str.assign(
				(std::istreambuf_iterator<char>(fileIn)),
				std::istreambuf_iterator<char>());
			return str;
		}
		bool isFirstIdentCharacter(int ch) { return std::isalpha(ch) || (ch == '_'); }
		bool isIdentCharacter(int ch) { return std::isalnum(ch) || (ch == '_'); }
	}

#define PARSE_TRY(p) { if (!(p)) { popctx(); return false; } }
	constexpr auto WHITESPACE = " \t\n\r";
	constexpr auto DELIMITERS = "{}[](),;.:+-*/!\\<>\n\r\t ";

	// Helper class for formatting parser error messages
	struct ParseErrorFormatter
	{
		ParseErrorFormatter(std::ostream& os_) : os(os_)
		{}

		~ParseErrorFormatter() {
			os << std::endl;
		}

		template <typename T>
		ParseErrorFormatter& operator<<(T&& value) {
			os << std::forward<T>(value);
			return *this;
		}

		std::ostream& os;
	};

	// Reusable parser class
	class ParserBase
	{
	public:
		static constexpr int END_OF_FILE = -1;

		struct ParseCtx
		{
			int line;
			int col;
			int pos;
		};

		ParserBase(const char *path)
		{
            ctx.push_back(ParseCtx{ 1, 1, 0 });
			insertSource(loadSource(path));
            includeDir = filesystem::path(path).parent_path();
		}

		ParseCtx& currentContext()
		{
			return ctx[ctx.size() - 1];
		}

		void insertSource(std::string src)
		{
            source.insert(currentContext().pos, std::move(src));
		}

        void insertInclude(const char* path)
        {
            auto srcPath = includeDir / path;
            source.insert(currentContext().pos, std::move(loadSource(srcPath.str().c_str())));
        }

		// Save parsing context
		void pushctx()
		{
			ctx.push_back(ctx.back());
		}

		// Restore parsing context from top of stack (backtrack)
		void popctx()
		{
			ctx.pop_back();
		}

		// replace context n-1 with the last context on the stack
		// pop the last context
		void commitctx()
		{
			ctx[ctx.size() - 2] = ctx[ctx.size() - 1];
			ctx.pop_back();
		}

		int getchRaw()
		{
			auto& ctx = currentContext();
			if (ctx.pos == source.size()) {
				return END_OF_FILE;
			}
			auto p = ctx.pos;
			// advance position 
			ctx.pos++;
			// next column
			ctx.col++;
			// handle end-of-lines
			if (source[p] == '\n') {
				ctx.col = 1;
				ctx.line++;
			}
			return source[p];
		}

		// get next character
		int getch()
		{
			skipLineComment();
			return getchRaw();
		}

		// look next character
		int lookch(int off = 0)
		{
			auto& ctx = currentContext();
			if (ctx.pos + off >= source.size()) {
				return END_OF_FILE;
			}
			return source[ctx.pos + off];
		}

		// match one char
		template <typename Pred>
		bool ppredch(char& c, Pred pred)
		{
			if (pred(lookch())) {
				c = (char)getch();
				return true;
			}
			return false;
		}


		// match zero or more characters, always succeeds
		template <typename Pred>
		void ppred(std::string& out, Pred pred)
		{
			while (pred(lookch())) {
				out += (char)getch();
			}
		}

		// Tries to parse an identifier string
		bool pident(std::string& out)
		{
			pushctx();
			char first;
			PARSE_TRY(ppredch(first, [](int ch) {return std::isalpha(ch) || (ch == '_');}));
			out += first;
			ppred(out, [](int ch) {return std::isalpha(ch) || (ch == '_');});
			commitctx();
			return true;
		}


		// Skip one or more whitespace
		bool skipws1()
		{
			if (!std::isblank(lookch())) {
				return false;
			}
			getch();
			skipws();
			return true;
		}

		// Skip whitespace
		void skipws()
		{
			while (std::isspace(lookch())) {
				getch();
			}
		}

		// Parse a character
		bool pchar(char c)
		{
			int nc = lookch();
			if (nc != c) {
				return false;
			}
			getch();
			return true;
		}

		// Parse a string
		bool pstring(std::string s)
		{
			pushctx();
			for (auto c : s) {
				if (c != getch()) {
					popctx();
					return false;
				}
			}

			commitctx();
			return true;
		}

		// Parse a token (string followed by a delimiter, does not consume the separator)
		bool ptoken(std::string s)
		{
			pushctx();
			PARSE_TRY(pstring(s));
			int next = lookch();
			if (!std::strchr(DELIMITERS, next)) {
				popctx();
				return false;
			}

			commitctx();
			return true;
		}

		// Parse until EOF or the specified character is found (but not consumed)
		void puntil(char c, std::string& s)
		{
			for (int next = lookch(); next != END_OF_FILE && next != c; next = lookch()) {
				s += (char)getch();
			}
		}

		// Parse until EOF or the specified character is found (but not consumed)
		void skipuntil(char c)
		{
			for (int next = lookch(); next != END_OF_FILE && next != c; next = lookch()) {
				getch();
			}
		}

		// Parse until next matching brace is found (or EOF)
		// returns false if the brace is not matched
		bool parseUntilMatchingBrace(char lbrace, std::string& s)
		{
			pushctx();
			char rbrace;
			switch (lbrace)
			{
			case '(': rbrace = ')'; break;
			case '{': rbrace = '}'; break;
			case '[': rbrace = ']'; break;
			case '<': rbrace = '>'; break;
			default: throw std::logic_error("Unrecognized brace character"); break;
			}

			int stack = 1;

			for (int next = lookch(); next != END_OF_FILE; next = lookch())
			{
				if (next == lbrace) {
					stack++;
					if (stack == 0) break;
				}
				if (next == rbrace) {
					stack--;
					if (stack == 0) break;
				}
				s += getch();
			}

			if (stack != 0) {
				popctx();
				return false;
			}

			commitctx();
			return true;
		}

		// Parse a (base 10) number
		bool pnum(int& num)
		{
			std::string numstr;
			// (0..9)+ 
			if (!std::isdigit(lookch())) {
				return false;
			}
			numstr += (char)getch();
			while (std::isdigit(lookch())) {
				numstr += (char)getch();
			}

			num = std::stoi(numstr);
			return true;
		}

		// parse end-of-file
		bool peof()
		{
			return lookch() == END_OF_FILE;
		}

		// overridable function to detect comments
		virtual void skipLineComment()
		{
			if (lookch() == '/' && lookch(1) == '/') {
				while (lookch() != '\n') {
					getchRaw();
				}
			}
		}

		ParseErrorFormatter parseError()
		{
			return ParseErrorFormatter(std::clog);
		}

	protected:
		// context stack
		std::vector<ParseCtx> ctx;
		// full source (concatenation of all includes)
		std::string source;
        // base source path
        filesystem::path includeDir;
	};
}
