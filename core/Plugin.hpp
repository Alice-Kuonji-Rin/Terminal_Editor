
#pragma once
#include "TextBuffer.hpp"
#include <vector>
#include <string>

struct HighlightRequest
{
	int row;
	int col;
	std::string color_color;  //例如：green color : \'x1b[42m'
};

class IPlugin
{
	virtual ~IPlugin() = default;

	virtual std::string getName() const = 0;

	virtual std::vector<HighlightRequest> getHighlights(TextBuffer* buffer, int cursor_col) = 0;

	virtual bool onKeyPress(TextBuffer* buffer, int& cursor_row, int& cursor_col, char key) { return false;}
};

typedef IPlugin* (*CreatePluginFunc)();
typedef void (*DestroyPluginFunc)(IPlugin*);