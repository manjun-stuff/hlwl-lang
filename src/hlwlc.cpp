#include <string>
#include <sstream>
#include <fstream>
#include <vector>
#include <iostream>
#include <cctype>
#include <unordered_map>
#include <filesystem>

namespace fs = std::filesystem;

// helpers
std::string get_line(const std::string &code, size_t lineIndex)
{
    std::istringstream stream(code);
    std::string currentLine;
    int currentLineNum = 1;

    while (std::getline(stream, currentLine))
    {
        if (currentLineNum == lineIndex)
        {
            return currentLine;
        }

        currentLineNum += 1;
    }

    return "Error";
}

std::string find_line(const std::string &code, const std::string &lineData)
{
    std::istringstream stream(code);
    std::string currentLine;

    while (std::getline(stream, currentLine))
    {
        if (currentLine.find(lineData) != std::string::npos)
        {
            return currentLine;
        }
    }

    return "Error";
}

int get_line_num(const std::string &code, const std::string &lineData)
{
    std::istringstream stream(code);
    std::string currentLine;
    int currentLineNum = 1;

    while (std::getline(stream, currentLine))
    {
        if (currentLine.find(lineData) != std::string::npos)
        {
            return currentLineNum;
        }
        ++currentLineNum;
    }
    return 1;
}

// Compiler namespace with notes so I know what is going on
namespace Compiler
{
    struct Diagnostic;

    enum class TokenType
    {
        // keywords
        VoidKw,
        RefKw,
        // identifiers & literals
        Identifier,
        StringLiteral,
        HexColor,
        // punctuation & operators
        LParen,
        RParen,
        LBrace,
        RBrace,
        Colon,
        Dot,
        Equals,
        Semicolon,
        // end of file
        EndOfFile
    };

    std::string token_type_to_string(TokenType type)
    {
        switch (type)
        {
        case TokenType::VoidKw:
            return "VoidKw";
        case TokenType::RefKw:
            return "RefKw";
        case TokenType::Identifier:
            return "Identifier";
        case TokenType::StringLiteral:
            return "StringLiteral";
        case TokenType::HexColor:
            return "HexColor";
        case TokenType::LParen:
            return "LParen";
        case TokenType::RParen:
            return "RParen";
        case TokenType::LBrace:
            return "LBrace";
        case TokenType::RBrace:
            return "RBrace";
        case TokenType::Colon:
            return "Colon";
        case TokenType::Dot:
            return "Dot";
        case TokenType::Equals:
            return "Equals";
        case TokenType::Semicolon:
            return "Semicolon";
        case TokenType::EndOfFile:
            return "EndOfFile";
        default:
            return "Unknown";
        }
    }

    struct Token
    {
        TokenType tokenType;
        std::string data;

        int line;
        int column;
    };

    /**
     * The output of the preprocessor that the other systems consume
     */
    struct Data
    {
        std::string pageName;
        fs::path outputDir;
        std::string rawData;
        std::vector<Diagnostic> diagnostics;
    };

    struct Diagnostic
    {
        std::string error;
        int line;
        int column;
    };

    class Preprocessor
    {
    public:
        std::string sourceCode;
        std::string resultPath;

        /**
         * Run the preprocessor on the file
         */
        Data process_file(const std::string &filePath)
        {
            Data outputData;
            read_file(filePath);

            std::string pageDecl = find_line(sourceCode, "<page");

            if (pageDecl == "Error")
            {
                outputData.diagnostics.push_back(Diagnostic{.error = "Missing <page, ...> declaration", .line = 1, .column = 1});
                return outputData;
            }

            size_t namePos = pageDecl.find("name");
            if (namePos == std::string::npos)
            {
                outputData.diagnostics.push_back(Diagnostic{"Missing required name argument of <page, ...> declaration", 1, 1});
                return outputData;
            }

            size_t openQuotePos = pageDecl.find('"', namePos);
            size_t closeQuotePos = pageDecl.find('"', openQuotePos + 1);

            if (openQuotePos == std::string::npos || closeQuotePos == std::string::npos)
            {
                return outputData;
            }

            std::string pageName = pageDecl.substr(openQuotePos + 1, closeQuotePos - openQuotePos - 1);

            fs::path outputDir = create_dir(pageName);

            outputData = {
                .pageName = pageName,
                .outputDir = outputDir,
                .rawData = "",
                .diagnostics = outputData.diagnostics};

            // step through tokens
            std::vector<std::string> entryFunctions;
            std::string sceneName;
            bool inComment = false;
            bool inTag = false;
            std::string tagData;
            bool inDoubleStr = false;
            bool inSingleStr = false;
            for (int i = 0; i < sourceCode.length(); i++)
            {
                char currChar = sourceCode[i];

                if (currChar == '"' && !inSingleStr)
                {
                    inDoubleStr = !inDoubleStr;
                }

                if (currChar == '\'' && !inDoubleStr)
                {
                    inSingleStr = !inSingleStr;
                }

                if (!inDoubleStr && !inSingleStr && currChar == '/' && i + 1 < sourceCode.length() && sourceCode[i + 1] == '/')
                {
                    inComment = true;
                }

                if (!inDoubleStr && !inSingleStr && currChar == '\n')
                {
                    inComment = false;
                }

                if (currChar == '<' && !inDoubleStr && !inSingleStr)
                {
                    inTag = true;
                    tagData = "";
                    continue;
                }

                if (inComment)
                    continue;

                if (currChar == '>' && !inDoubleStr && !inSingleStr)
                {
                    inTag = false;
                    // inspect tagdata
                    if (tagData.find("scene") != std::string::npos && tagData[0] != '/')
                    {
                        size_t namePos = tagData.find("name");
                        if (namePos == std::string::npos)
                        {
                            outputData.diagnostics.push_back(Diagnostic{"Missing required name argument of <scene, ...> declaration", 1, 1});
                        }

                        size_t openQuotePos = tagData.find('"', namePos);
                        size_t closeQuotePos = tagData.find('"', openQuotePos + 1);

                        if (openQuotePos == std::string::npos || closeQuotePos == std::string::npos)
                        {
                            outputData.diagnostics.push_back(Diagnostic{"Missing required name of <scene, ...> declaration", 1, 1});
                            continue;
                        }

                        sceneName = tagData.substr(openQuotePos + 1, closeQuotePos - openQuotePos - 1);
                        for (char &c : sceneName)
                        {
                            if (c == ' ')
                            {
                                c = '_';
                            }
                        }
                    }

                    if (tagData.find("layout") != std::string::npos && tagData[0] != '/')
                    {
                        std::string funcName = "__layout_" + sceneName + "__";
                        entryFunctions.push_back(funcName);
                        outputData.rawData += "void " + funcName + "() {\n";
                    }

                    if (tagData.find("/layout") != std::string::npos)
                    {
                        outputData.rawData += "}";
                    }

                    continue;
                }

                if (inTag)
                    tagData += currChar;

                if (!inTag)
                    outputData.rawData += currChar;
            }

            outputData.rawData += "\nvoid __entry__() {\n";
            for (const std::string &funcName : entryFunctions)
            {
                outputData.rawData += "    " + funcName + "();\n";
            }
            outputData.rawData += "}\n";

            return outputData;
        }

    private:
        void read_file(const std::string &filePath)
        {
            std::ifstream inFile(filePath);
            std::stringstream buffer;
            buffer << inFile.rdbuf();
            sourceCode = buffer.str();
        }

        fs::path create_dir(const std::string &projectName)
        {
            fs::create_directories(projectName);

            fs::path outputDir = projectName;

            std::ofstream(outputDir / "index.html");
            std::ofstream(outputDir / "style.css");
            std::ofstream(outputDir / "index.js");

            return outputDir;
        }
    };

    class Lexer
    {
    public:
        std::vector<Token> tokenize_data(Data &inputData)
        {
            pos = 0;
            line = 1;
            column = 1;

            std::vector<Token> tokens = {};
            if (inputData.rawData == "")
                return tokens;
            source = inputData.rawData;

            do
            {
                char currChar = source[pos];

                if (std::isspace(currChar))
                {
                    advance();
                    continue;
                }

                if (std::isalpha(currChar) || currChar == '_')
                {
                    std::string word = std::string(1, currChar);
                    currChar = advance();

                    while ((std::isalnum(currChar) || currChar == '_') && !is_at_end())
                    {
                        word += currChar;
                        currChar = advance();
                    }

                    if (keywords.contains(word))
                    {
                        tokens.push_back(Token{.data = word, .line = line, .column = column, .tokenType = keywords.at(word)});
                    }
                    else
                    {
                        tokens.push_back(Token{.data = word, .line = line, .column = column, .tokenType = TokenType::Identifier});
                    }

                    continue;
                }

                if (currChar == '"')
                {
                    std::string word = "";
                    if (!is_at_end())
                        currChar = advance();

                    while (currChar != '"' && !is_at_end())
                    {
                        word += currChar;
                        currChar = advance();
                    }

                    tokens.push_back(Token{
                        .data = word,
                        .line = line,
                        .column = column,
                        .tokenType = TokenType::StringLiteral});

                    if (!is_at_end())
                        advance();
                    continue;
                }

                if (currChar == '#')
                {
                    std::string word = "";
                    if (!is_at_end())
                        currChar = advance();

                    while (std::isxdigit(currChar) && !is_at_end())
                    {
                        word += currChar;
                        currChar = advance();
                    }

                    tokens.push_back(Token{
                        .data = word,
                        .line = line,
                        .column = column,
                        .tokenType = TokenType::HexColor});

                    continue;
                }

                Token token;
                token.column = column;
                token.line = line;
                token.data = "";

                switch (currChar)
                {
                case '{':
                    token.data = "{";
                    token.tokenType = TokenType::LBrace;
                    break;
                case '}':
                    token.data = "}";
                    token.tokenType = TokenType::RBrace;
                    break;
                case '(':
                    token.data = "(";
                    token.tokenType = TokenType::LParen;
                    break;
                case ')':
                    token.data = ")";
                    token.tokenType = TokenType::RParen;
                    break;
                case ':':
                    token.data = ":";
                    token.tokenType = TokenType::Colon;
                    break;
                case '.':
                    token.data = ".";
                    token.tokenType = TokenType::Dot;
                    break;
                case '=':
                    token.data = "=";
                    token.tokenType = TokenType::Equals;
                    break;
                case ';':
                    token.data = ";";
                    token.tokenType = TokenType::Semicolon;
                    break;
                }

                if (token.data != "")
                {
                    tokens.push_back(token);
                }
                else
                {
                    inputData.diagnostics.push_back(Diagnostic{
                        .error = "Unexpected character",
                        .line = line,
                        .column = column});
                    if (!is_at_end())
                        advance();
                }

                if (!is_at_end())
                    advance();

            } while (!is_at_end());

            tokens.push_back(Token{
                .data = "",
                .column = column,
                .line = line,
                .tokenType = TokenType::EndOfFile});

            return tokens;
        }

    private:
        std::string source;
        size_t pos = 0;
        int line = 1;
        int column = 1;

        std::unordered_map<std::string, TokenType> keywords = {
            {"void", TokenType::VoidKw},
            {"r", TokenType::RefKw}};

        bool is_at_end() const
        {
            return pos >= source.length();
        }

        char advance()
        {
            char currChar = source[++pos];
            if (currChar == '\n')
            {
                line++;
                column = 1;
            }
            else
            {
                column++;
            }
            return currChar;
        }
    };

    class Parser
    {
    };

    class Emitter
    {
    };

    void print_errors(Data data)
    {
        for (int i = 0; i < data.diagnostics.size(); ++i)
        {
            Diagnostic currDiagnostic = data.diagnostics[i];

            std::cerr << "Error [" << currDiagnostic.line << ", " << currDiagnostic.column << "]: " << currDiagnostic.error << std::endl;
        }
    }
}