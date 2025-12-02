#pragma once
#include "../core/AbstractEditor.hpp"
#include "../core/PluginLoader.hpp"
#include "../features/CommandShell.hpp"
#include <iostream>
#include <chrono>

enum EditorMode
{
	MODE_NORMAL,
	MODE_INSERT,
	MODE_COMMAND
};


class SimpleTextEditor : public AbstractEditor
{
public:
	void drawInterface() override;
	void processInput() override;
	void run() override;

	SimpleTextEditor(); //构造函数初始化模式

	//新增供CommandShell 调用的接口
	void setShouldQuit(bool quit) { should_quit = quit; }
	void updateStatusMessage(const std::string& msg) { status_msg = msg; }
	void saveFile(); // to let the save logic move to simpleTextEditor::saveFile();
	std::string getFilename() const;


private:

	EditorMode mode;	//当前模式
	std::string status_msg;		//状态栏显示的消息

	CommandShell shell;			//命令解析器实例
	std::string commandBuffer;   //命令行的输入缓存区


	//拆分输入处理
	void processNormalKeyPress();  
	void processInsertKeyPress();
	void processCommandKeyPress();

	//辅助显示状态栏
	void drawStatusBar();	
	void drawCommandLine();   //新增绘制命令行

	void editorScroll();	//滚动处理

	//用于实现jk退出功能的记录变量
	char last_key = 0;
	std::chrono::steady_clock::time_point last_key_time;

	PluginManager pluginManager;
};