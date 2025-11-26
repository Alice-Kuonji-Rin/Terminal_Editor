#include "TerminalController.hpp"

TerminalController::TerminalController()
{
	//构造时可以暂不开启 Raw Mode，由 Editor 控制
}

TerminalController::~TerminalController()
{
	disableRawMode();
}

void TerminalController::enableRawMode()
{
	if(inRawMode) return;
	if(tcgetattr(STDIN_FILENO, &original_termios) == -1)
	{
		throw std::runtime_error("Failed to get terminal settings");
	}

	struct termios raw = original_termios;
	raw.c_lflag &= ~(ECHO | ICANON | ISIG | IEXTEN);
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    raw.c_oflag &= ~(OPOST);
    raw.c_cflag |= (CS8);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 1;

    if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1)
    {
    	throw std::runtime_error("Failed to set raw mode");
    }
    inRawMode = true;
}


void TerminalController::disableRawMode()
{
	if(!inRawMode) return;
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &original_termios);
	inRawMode = false;
}


TerminalSize TerminalController::getTerminalSize()
{
	struct winsize ws;
	if(ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0)
	{
		return {24, 80};
	}
	return {ws.ws_row, ws.ws_col};
}

void TerminalController::clearScreen()
{
	std::cout << "\x1b[2J\x1b[H" << std::flush;
}

void TerminalController::moveCursor(int row, int col)
{
	std::cout << "\x1b[" << row << ";" << col << "H" << std::flush;
}

void TerminalController::hideCursor()
{
	std::cout << "\x1b[?25l" << std::flush;
}

void TerminalController::showCursor() 
{
    std::cout << "\x1b[?25h" << std::flush;
}

void TerminalController::writeString(const std::string& str) 
{
    std::cout << str << std::flush;
}

void TerminalController::enterAlternateScreen() 
{
    std::cout << "\x1b[?1049h\x1b[H" << std::flush;
}

void TerminalController::exitAlternateScreen() 
{
    std::cout << "\x1b[?1049l" << std::flush;
}












































