#pragma once
#include <string>
#include <vector>
#include <functional>
#include <unordered_map>
#include <memory> // 修正拼写: memeory -> memory
#include <sstream>

class SimpleTextEditor;

enum class TokenType
{
	COMMAND,
	ARGUMENT,
	SEPARATOR,
	END_OF_INPUT
};

struct Token
{
	TokenType type;
	std::string value;
}; // 修正: 补全分号

struct CommandContext
{
	SimpleTextEditor* editor;
	std::vector<std::string> args;
}; // 修正: 补全分号

class CommandShell
{
public:
	using CommandHandler = std::function<void(CommandContext&)>;
	CommandShell();

    // 修正: 补全行末分号
	void registerCommand(const std::string& cmd, CommandHandler handler); 
	void execute(const std::string& input, SimpleTextEditor* editor);

private:
	std::vector<Token> tokenize(const std::string& input);

	void parseAndRun(const std::vector<Token>& tokens, SimpleTextEditor* editor);
	std::unordered_map<std::string, CommandHandler> commandRegistry;
};