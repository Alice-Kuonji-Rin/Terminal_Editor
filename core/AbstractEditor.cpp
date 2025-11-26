#include "AbstractEditor.hpp"


AbstractEditor::AbstractEditor()
{
	terminal = std::make_unique<TerminalController>();
	buffer = std::make_unique<TextBuffer>();
}

AbstractEditor::~AbstractEditor()
{
	terminal->exitAlternateScreen();
	terminal->showCursor();
	terminal->disableRawMode();
}

void AbstractEditor::updateTerminalSize()
{
	current_size = terminal->getTerminalSize();
}

void AbstractEditor::run()
{
	terminal->enableRawMode();
	terminal->enterAlternateScreen();
	updateTerminalSize();
	
	while(!should_quit)
	{
		drawInterface();
		processInput();
		usleep(10000);
	}
}

void AbstractEditor::refreshScreen()
{
	//基础的刷新逻辑可以放这里，或者由 drawInteface 完全接管
	//这里留空， 让子类决定如何刷新
}


















