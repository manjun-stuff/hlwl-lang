#include "hlwlc.cpp"
#include <filesystem>
#include <fstream>
#include <iostream>

namespace fs = std::filesystem;

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        std::cerr << "Usage: hlwl <file.hlwl>\n";
        return 1;
    }

    std::string filePath = argv[1];

    if (!fs::exists(filePath))
    {
        std::cerr << "Error: File not found -> " << filePath << "\n";
        return 1;
    }

    Compiler::Preprocessor preprocessor;
    Compiler::Data data = preprocessor.process_file(filePath);

    Compiler::Lexer lexer;
    std::vector<Compiler::Token> tokens = lexer.tokenize_data(data);

    if (!data.diagnostics.empty())
    {
        Compiler::print_errors(data);
    }

    std::ofstream hlwtFile(data.outputDir / "test.txt");

    if (!hlwtFile.is_open())
    {
        std::cerr << "Error opening file" << std::endl;
    }

    // make human readable form of tokens
    std::string textTokens = "";
    for (int i = 0; i < tokens.size(); ++i)
    {
        Compiler::Token currToken = tokens[i];
        textTokens += Compiler::token_type_to_string(currToken.tokenType) + ": " + currToken.data + "\n";
    }

    hlwtFile << textTokens;

    hlwtFile.close();

    return 0;
}