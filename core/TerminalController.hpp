#pragma once
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <iostream>
#include <stdexcept>


struct TerminalSize
{
	int rows;
	int cols;
};

class TerminalController
{
public:
	TerminalController();
	~TerminalController();

	TerminalSize getTerminalSize();
	void enableRawMode();		//开启原始模式，让编辑器能够按字节实时处理输入，并完全掌控屏幕输出。
	void disableRawMode();		//恢复终端到程序运行前到状态

	void moveCursor(int row, int col);	//将光标精确定位到指定的行列
	void hideCursor();				//在重绘屏幕期间隐藏光标，重绘完成后再显示。
	void showCursor();
	void writeString(const std::string& str);	//统一的输出接口

	void clearScreen();			//清空整个屏幕内容，并将光标移动的左上角
	void enterAlternateScreen();
	void exitAlternateScreen();
private:
	struct termios original_termios;
	bool inRawMode = false;
};