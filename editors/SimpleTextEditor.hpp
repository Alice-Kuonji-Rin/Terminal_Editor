#pragma once
#include "../core/AbstractEditor.hpp"
#include <iostream>


enum EditorMode
{
	MODE_NORMAL,
	MODE_INSERT
};


class SimpleTextEditor : public AbstractEditor
{
public:
	void drawInterface() override;
	void processInput() override;
	void run() override;

	SimpleTextEditor(); //构造函数初始化模式

private:

	EditorMode mode;	//当前模式
	std::string status_msg;		//状态栏显示的消息

	//拆分输入处理
	void processNormalKeyPress();  
	void processInsertKeyPress();

	//辅助显示状态栏
	void drawStatusBar();	

	void editorScroll();	//滚动处理


};