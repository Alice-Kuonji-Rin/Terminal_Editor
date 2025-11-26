#pragma once
#include "TerminalController.hpp"
#include "TextBuffer.hpp"
#include <memory>
#include <unistd.h>

class AbstractEditor
{
public:
	AbstractEditor();

	virtual ~AbstractEditor();
	virtual void drawInterface() = 0;
	virtual void processInput() = 0;
	virtual void run() = 0;
	

protected:

	std::unique_ptr<TerminalController> terminal;
	std::unique_ptr<TextBuffer> buffer;

	int cursor_row = 0;
	int cursor_col = 0;
	int row_offset = 0;
	int col_offset = 0;
	bool should_quit = false;
	TerminalSize current_size;

	void updateTerminalSize();
	void editorScroll();
	void refreshScreen();

};
