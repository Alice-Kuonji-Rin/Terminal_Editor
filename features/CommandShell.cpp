#include "CommandShell.hpp"
#include "../editors/SimpleTextEditor.hpp"
#include <iostream>
#include <algorithm>
#include <sstream>


std::string toLower(const std::string& str)
{
	std::string result = str;
	std::transform(result.begin(), result.end(), result.begin(), [](unsigned char c){ return std::tolower(c);});
	return result;
}



CommandShell::CommandShell()
{
	// ---在构造函数中注册所用命令
	//这种方式极易扩展，新增命令只需加一行代码， 不需改变核心逻辑


	//保存命令 :w
	registerCommand("w", [](CommandContext& ctx){
		if(ctx.args.empty())
		{
			ctx.editor->saveFile();
		}
		else
		{
			ctx.editor->updateStatusMessage("Command: Save As to " + ctx.args[0] + " (Not Implemented)");
		}});

	//退出命令 :q
	registerCommand("q", [](CommandContext& ctx){
		if(ctx.args.size() == 1 && ctx.args[0] == "!")
		{
			ctx.editor->setShouldQuit(true);
		}
		else
		{
			ctx.editor->setShouldQuit(true);
		}});

	//保存并退出
	registerCommand("wq", [](CommandContext& ctx){
		ctx.editor->saveFile();
		ctx.editor->setShouldQuit(true);});
}


void CommandShell::registerCommand(const std::string& cmd, CommandHandler handler)
{
	commandRegistry[cmd] = handler;
}


void CommandShell::parseAndRun(const std::vector<Token>& tokens, SimpleTextEditor* editor)
{
	if(tokens.empty()) return;

	std::string commandName = toLower(tokens[0].value);
	std::vector<std::string> args;

	if(tokens.size() > 1)
	{
		for(size_t i = 1; i < tokens.size(); i++)
		{
			args.emplace_back(tokens[i].value);
		}
	}

	auto it = commandRegistry.find(commandName);
	if(it != commandRegistry.end())
	{
		CommandContext ctx;
		ctx.editor = editor;
		ctx.args = args;

		it->second(ctx);
	}
	else
	{
		editor->updateStatusMessage("Error: Unknown command: " + commandName);
	}
}

void CommandShell::execute(const std::string& input, SimpleTextEditor* editor)
{
	auto tokens = tokenize(input);
	parseAndRun(tokens, editor);
}

std::vector<Token> CommandShell::tokenize(const std::string& input)
{
	std::vector<Token> tokens;
	std::istringstream iss(input);
	std::string segment;

	std::string cleanInput = input;
	if(!cleanInput.empty() && cleanInput[0] == ':')
	{
		cleanInput = cleanInput.substr(1);
	}
	std::istringstream iss_clean(cleanInput);

	bool isFirst = true;
	while(iss_clean >> segment)
	{
		Token t;
		t.value = segment;
		if(isFirst)
		{
			t.type = TokenType::COMMAND;
			isFirst = false;
		}
		else
		{
			t.type = TokenType::ARGUMENT;
		}
		tokens.emplace_back(t);
	}
	return tokens;
}
















