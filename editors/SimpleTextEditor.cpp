#include "SimpleTextEditor.hpp"

#include <cstdio>
#include <string>
#include <unistd.h>
#include <ctype.h>


SimpleTextEditor::SimpleTextEditor()
{
    mode = MODE_NORMAL;  //默认进入 Normal 模式

    buffer->setFilename("untitled.txt");    // 假设我们在 main.cpp 里会设置文件名，这里先给个默认
}


void SimpleTextEditor::processInput() 
{
    if(mode == MODE_NORMAL)
    {
        processNormalKeyPress();
    }
    else
    {
        processInsertKeyPress();
    }
}



void SimpleTextEditor::processNormalKeyPress()
{
    char c;
    if(read(STDIN_FILENO, &c, 1) == -1) return;

    if(c == 'i')
    {
        mode = MODE_INSERT;
        status_msg = "-- INSERT --";
        return;
    }

    if(c == 'q')
    {
        should_quit = true;
        return;
    }


    if(c == 'w')
    {
        buffer->saveToFile();
        status_msg = "File Save: " + buffer->getFilename();
    }


    // --- 移动光标 (hjkl) ---
    // 这里我们简单映射 hjkl 到原来的坐标变量
    if (c == 'h') { if (cursor_col > 0) cursor_col--; }
    else if (c == 'j') { if (cursor_row < buffer->getLineCount() - 1) cursor_row++; }
    else if (c == 'k') { if (cursor_row > 0) cursor_row--; }
    else if (c == 'l') { if (cursor_col < buffer->getLineLength(cursor_row)) cursor_col++; }

    editorScroll();        // 处理一下滚动
    buffer->setDirty(true);  // 强制重绘以更新光标或状态
}



void SimpleTextEditor::processInsertKeyPress()
{
    char c;
    if(read(STDIN_FILENO, &c, 1) == -1) return;

    if(c == '\x1b')
    {
        mode = MODE_NORMAL;
        status_msg = "-- NORMAL --";
        if(cursor_col > 0) cursor_col--;
        return;
    }

    else if (c == 127) {
        if (cursor_col > 0) {
            buffer->deleteChar(cursor_row, cursor_col - 1);
            cursor_col--;
            
        } else if (cursor_row > 0) {
            int prev_len = buffer->getLineLength(cursor_row - 1);
            buffer->mergeLine(cursor_row);
            cursor_row--;
           
            cursor_col = prev_len;
        }
    }

    // Enter (13 or 10)
    else if (c == 13 || c == 10) {
        buffer->splitLine(cursor_row, cursor_col);
        cursor_row++;
        cursor_col = 0;
    }
    
    // 普通字符输入
    else if (!iscntrl(c)) {
        buffer->insertChar(cursor_row, cursor_col, c);
        cursor_col++;
    }
    
    // 统一处理滚动和脏标记
    editorScroll();
    if(!should_quit)
    {
         buffer->setDirty(true);
    }
}




void SimpleTextEditor::drawInterface()
{
	if(!buffer->isDirty()) return;

	const TerminalSize& size = current_size;
	terminal->hideCursor();
	terminal->moveCursor(1, 1);

	// --- 绘制标题栏 ---
	//std::string title = "C++ Text Editor - Size: " + std::to_string(size.cols) + "x" + std::to_string(size.rows);  original type

    std::string title = "Size: " + std::to_string(size.cols) + "x" + std::to_string(size.rows);
    if(title.length() > size.cols) title = title.substr(0, size.cols);
	if(title.length() > size.cols) title = title.substr(0, size.cols);


	std::cout << "\x1b[44m\x1b[37m" << title;
	for(int i = title.length(); i < size.cols; i++) std::cout << " ";
	std::cout << "\x1b[0m\r\n";


	// --- 绘制内容 ---
	int content_rows = size.rows - 2;
	for(int screen_row = 0; screen_row < content_rows; screen_row++)
	{
		int file_row = screen_row + row_offset;
		if(file_row < buffer -> getLineCount())
		{
			std::string line = buffer->getLine(file_row);

            if(col_offset < static_cast<int>(line.length()))
            {
                std::string visible = line.substr(col_offset, size.cols);
                std::cout << visible;
            }
			std::cout << "\x1b[K";

		}
		else
		{
			std::cout << "~" << "\x1b[K";
		}
		std::cout << "\r\n";        
	}

    
    // --- 绘制状态栏 ---
    // 移动到倒数第二行（或者最后一行）
    terminal->moveCursor(size.rows, 1); 
    // 反色显示
    std::cout << "\x1b[7m"; 

    // 准备状态文字：模式 + 文件名 + 光标位置
    std::string status = " " + status_msg + " | " + buffer->getFilename() + 
                         " | " + std::to_string(cursor_row) + ":" + std::to_string(cursor_col);

    // 补全空格铺满整行
    while(status.length() < size.cols) status += " ";
    // 截断防止溢出
    if(status.length() > size.cols) status = status.substr(0, size.cols);

    std::cout << status << "\x1b[0m"; // 打印并恢复颜色

  
	// --- 恢复光标位置 --- 
	// +2 是因为有标题栏，且 ANSI 坐标从 1 开始
	int cursor_screen_row = cursor_row - row_offset + 2;
	int cursor_screen_col = cursor_col - col_offset + 1;

    if(cursor_screen_row < 2) cursor_screen_row = 2;
    if(cursor_screen_col < 1) cursor_screen_col = 1;

	terminal->moveCursor(cursor_screen_row, cursor_screen_col);
	terminal->showCursor();

	buffer->setDirty(false);
}


void SimpleTextEditor::editorScroll()
{
    // 1. 确保光标边界检查（光标不能超出文件实际范围）
    if (cursor_row >= buffer->getLineCount()) {
        cursor_row = buffer->getLineCount() > 0 ? buffer->getLineCount() - 1 : 0;
    }
    
    if (cursor_row < buffer->getLineCount()) {
        int len = buffer->getLineLength(cursor_row);
        if (cursor_col > len) {
            cursor_col = len;
        }
    } else {
        cursor_col = 0;
    }
    
    // 屏幕内容区域高度（除去标题栏和状态栏，假设共 2 行）
    int content_rows = current_size.rows - 2; 

    // 2. 垂直滚动（行）
    
    // 向上滚动：如果光标在当前屏幕显示的第一行上方
    if (cursor_row < row_offset) {
        row_offset = cursor_row;
    }

    // 向下滚动：如果光标在当前屏幕显示的最后一行下方
    // 屏幕可见的最后一行索引是 (content_rows - 1)。
    // 对应的文件行号是 row_offset + content_rows - 1。
    if (cursor_row >= row_offset + content_rows) {
        row_offset = cursor_row - content_rows + 1;
    }


    // 3. 水平滚动（列）
    
    // 向左滚动：如果光标在当前屏幕显示的左侧看不见
    if (cursor_col < col_offset) {
        col_offset = cursor_col;
    }

    // 向右滚动：如果光标在当前屏幕显示的右侧看不见
    // 屏幕可见列是 current_size.cols（假设没有垂直状态栏）
    if (cursor_col >= col_offset + current_size.cols) {
        col_offset = cursor_col - current_size.cols + 1;
    }
}
	


void SimpleTextEditor::run()
{
	AbstractEditor::run();	
}







/*void SimpleTextEditor::processInput()
{
    char c;
    if(read(STDIN_FILENO, &c, 1) == -1) return;

    if (c == 'q') 
    {
        // 只有当文件只有几行默认内容时才退出，或者强制退出
        should_quit = true;
    } 
    // 方向键处理 (简化版，仅作架构演示，实际需要完整解析器)
    else if (c == '\x1b') 
    {
        char seq[3];
        if (read(STDIN_FILENO, &seq[0], 1) == -1) return;
        if (read(STDIN_FILENO, &seq[1], 1) == -1) return;
        if (seq[0] == '[') {
            switch (seq[1]) {
                case 'A': 
                    if (cursor_row > 0) cursor_row--;  
                    break; // Up
                case 'B': 
                    if (cursor_row < buffer->getLineCount() - 1) cursor_row++;  
                    break; // Down
                case 'C': 
                    if(cursor_col < buffer->getLineLength(cursor_row)) cursor_col++;
                    else if(cursor_row < buffer->getLineCount() - 1)
                    {
                        cursor_row++;
                        cursor_col = 0;
                    }
                    break; // Right
                case 'D': 
                    if (cursor_col > 0) cursor_col--;  
                    else if(cursor_row > 0)
                    {
                        cursor_row--;
                        cursor_col = buffer->getLineLength(cursor_row);
                    }
                    break; // Left
            }
        }
    }

    // Backspace (127)
    else if (c == 127) {
        if (cursor_col > 0) {
            buffer->deleteChar(cursor_row, cursor_col - 1);
            cursor_col--;
            
        } else if (cursor_row > 0) {
            int prev_len = buffer->getLineLength(cursor_row - 1);
            buffer->mergeLine(cursor_row);
            cursor_row--;
           
            cursor_col = prev_len;
        }
    }

    // Enter (13 or 10)
    else if (c == 13 || c == 10) {
        buffer->splitLine(cursor_row, cursor_col);
        cursor_row++;
        cursor_col = 0;
    }
    
    // 普通字符输入
    else if (!iscntrl(c)) {
        buffer->insertChar(cursor_row, cursor_col, c);
        cursor_col++;
        
    }
    editorScroll();
    
    // 任何输入都标记为脏，触发重绘
    if(!should_quit)
    {
         buffer->setDirty(true);
    }   
}*/

























