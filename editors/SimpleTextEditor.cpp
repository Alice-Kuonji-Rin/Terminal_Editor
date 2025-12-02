#include "SimpleTextEditor.hpp"
#include <filesystem>
#include <cstdio>
#include <string>
#include <unistd.h>
#include <ctype.h>
#include <iostream>
#include <algorithm>

namespace fs = std::filesystem;

// --- 构造函数 ---
SimpleTextEditor::SimpleTextEditor() : shell() 
{
    mode = MODE_NORMAL;
    // 确保初始化时获取一次屏幕尺寸，防止第一次绘制计算错误
    updateTerminalSize(); 

    std::string pluginDir = "./plugins";
    if(fs::exists(pluginDir) && fs::is_directory(pluginDir))
    {
        std::cout << "Scanning plugins in: " << pluginDir << "\r\n";

        for(const auto& entry : fs::directory_iterator(pluginDir))
        {
            if(path.find(".dylib") != std::string::npos || path.find(".so") != std::string::npos)
            {
                pluginManager.loadPlugin(path);
            }
        }
    }
    else
    {
        // 如果目录不存在，可以创建它，或者忽略
        // fs::create_directory(pluginDir);
    }
}

// --- 核心入口：输入处理分发 ---
void SimpleTextEditor::processInput() 
{
    // 根据当前模式分发给不同的处理函数
    if(mode == MODE_NORMAL)
    {
        processNormalKeyPress();
    }
    else if(mode == MODE_INSERT)
    {
        processInsertKeyPress();
    }
    else if(mode == MODE_COMMAND)
    {
        processCommandKeyPress();
    }
    
    // 输入处理完后，统一检查滚动边界
    editorScroll(); 
}

// --- 核心入口：绘制界面 ---
void SimpleTextEditor::drawInterface()
{
    // 1. 脏标记检查：如果没有变化，不进行昂贵的重绘
    if(!buffer->isDirty()) return;
    
    // 2. 获取最新终端大小
    updateTerminalSize();
    const TerminalSize& size = current_size; 

    // 3. 准备画布：隐藏光标，移动到左上角
    terminal->hideCursor();
    terminal->moveCursor(1, 1);

    // ==========================================
    //           A. 绘制标题栏
    // ==========================================
    std::string title = "C++ Text Editor - Size: " + std::to_string(size.cols) + "x" + std::to_string(size.rows);
    if((int)title.length() > size.cols) title = title.substr(0, size.cols);

    std::cout << "\x1b[44m\x1b[37m" << title; // 蓝底白字
    for(int i = title.length(); i < size.cols; i++) std::cout << " ";
    std::cout << "\x1b[0m\r\n"; // 重置颜色并换行

    // ==========================================
    //           B. 收集插件高亮请求
    // ==========================================
    std::vector<HighlightRequest> highlights;
    
    // 遍历所有已加载的插件 (PluginManager)
    for (IPlugin* plugin : pluginManager.loadedPlugins) {
        // 询问插件：当前光标状态下，有哪些位置需要高亮？
        // 注意：buffer 是 unique_ptr，使用 .get() 获取原始指针传给插件
        auto reqs = plugin->getHighlights(buffer.get(), cursor_row, cursor_col);
        highlights.insert(highlights.end(), reqs.begin(), reqs.end());
    }

    // ==========================================
    //           C. 绘制文件内容
    // ==========================================
    int content_rows = size.rows - 2; // 除去标题栏(1)和底部UI(1)
    
    for(int screen_row = 0; screen_row < content_rows; screen_row++)
    {
        int file_row = screen_row + row_offset; // 计算实际文件行号

        if(file_row < buffer->getLineCount())
        {
            std::string line = buffer->getLine(file_row);
            int line_len = line.length();

            // 计算当前行在屏幕上可见的起始和结束列索引
            // col_offset 是水平滚动偏移量
            int start_col = col_offset;
            // 结束列 = 起始列 + 屏幕宽度 (不能超过行长)
            int end_col = std::min(line_len, col_offset + size.cols);

            // --- [关键修改] 逐字符绘制循环 ---
            // 为了支持高亮，必须一个字符一个字符地画
            for (int j = start_col; j < end_col; ++j) {
                char c = line[j];
                
                // 检查当前字符 (file_row, j) 是否在高亮列表中
                bool is_highlighted = false;
                std::string color_code = "";

                // 简单的线性查找 (因为高亮通常很少，性能影响不大)
                for (const auto& hl : highlights) {
                    if (hl.row == file_row && hl.col == j) {
                        color_code = hl.color;
                        is_highlighted = true;
                        break; // 找到一个高亮规则即可
                    }
                }
                
                // 1. 如果需要高亮，先设置颜色
                if (is_highlighted) {
                    std::cout << color_code;
                }
                
                // 2. 打印字符
                std::cout << c;
                
                // 3. 如果设置了颜色，立即重置，防止污染下一个字符
                if (is_highlighted) {
                    std::cout << "\x1b[0m"; 
                }
            }

            // 清除行尾残留的旧内容
            std::cout << "\x1b[K"; 
        }
        else
        {
            // 超出文件范围，绘制波浪线
            std::cout << "~" << "\x1b[K";
        }
        std::cout << "\r\n"; // 换行
    }

    // ==========================================
    //           D. 绘制底部 UI
    // ==========================================
    drawStatusBar();
    drawCommandLine();
    
    // ==========================================
    //           E. 恢复光标位置
    // ==========================================
    if (mode == MODE_COMMAND) {
        // 命令模式：光标在屏幕最底部，跟随输入内容
        terminal->moveCursor(current_size.rows, commandBuffer.length() + 1);
    } else {
        // 编辑模式：计算光标在屏幕上的位置
        // +2 是因为有标题栏(1行)，且ANSI坐标从1开始
        int cursor_screen_row = cursor_row - row_offset + 2; 
        int cursor_screen_col = cursor_col - col_offset + 1; 

        // 简单的屏幕边界保护
        if (cursor_screen_row < 2) cursor_screen_row = 2;
        if (cursor_screen_row > content_rows + 1) cursor_screen_row = content_rows + 1;

        terminal->moveCursor(cursor_screen_row, cursor_screen_col);
    }
    
    terminal->showCursor();

    // 重置脏标记，等待下一次更新
    buffer->setDirty(false);
}


// 1. 命令模式输入处理
void SimpleTextEditor::processCommandKeyPress()
{
    char c;
    if (read(STDIN_FILENO, &c, 1) == -1) return;

    // Enter: 执行命令
    if (c == 13 || c == 10) { 
        if (commandBuffer.length() > 1) { 
            // 将命令行内容传给 Shell 执行 (去除首位的 ':' 或 '%')
            shell.execute(commandBuffer, this);
        }
        
        // 退出命令模式
        commandBuffer.clear();
        mode = MODE_NORMAL;
    } 
    // Backspace: 删除字符
    else if (c == 127 || c == 8) { 
        if (commandBuffer.length() > 1) { 
            commandBuffer.pop_back();
        } else if (commandBuffer.length() == 1) {
            // 删除了 ':'，退回 Normal 模式
            commandBuffer.clear();
            mode = MODE_NORMAL;
        }
    }
    // ESC: 取消命令
    else if (c == 27) { 
        commandBuffer.clear();
        mode = MODE_NORMAL;
        status_msg = "Command cancelled";
    }
    // 普通字符: 追加到缓冲
    else if (!iscntrl(c)) { 
        commandBuffer += c;
    }
    
    buffer->setDirty(true);
}

// 2. Normal 模式输入处理
void SimpleTextEditor::processNormalKeyPress()
{
    char c;
    if(read(STDIN_FILENO, &c, 1) == -1) return;

    // --- 模式切换 ---
    if(c == ':') {
        mode = MODE_COMMAND;
        commandBuffer = ":";
        status_msg = "";
        buffer->setDirty(true);
        return;
    }
    if(c == 'i') {
        mode = MODE_INSERT;
        status_msg = "-- INSERT --";
        buffer->setDirty(true);
        return;
    }

    // --- 光标移动 (hjkl) ---
    // 修改光标位置，具体的滚动在 editorScroll 中处理
    if (c == 'k') { if (cursor_col > 0) cursor_col--; }
    else if (c == 'l') { if (cursor_row < buffer->getLineCount() - 1) cursor_row++; }
    else if (c == 'o') { if (cursor_row > 0) cursor_row--; }
    else if (c == ';') { 
        // 允许移动到当前行尾
        int len = buffer->getLineLength(cursor_row);
        if (cursor_col < len) cursor_col++; 
    }
    
    // 任何按键都触发重绘（简单起见）
    buffer->setDirty(true);
}

// 3. Insert 模式输入处理
void SimpleTextEditor::processInsertKeyPress()
{
    char c;
    // 注意：read 返回 0 表示超时无输入，返回 -1 表示错误
    if (read(STDIN_FILENO, &c, 1) <= 0) return;

    // 获取当前时间
    auto now = std::chrono::steady_clock::now();

    // --- [新增] jk 退出逻辑 ---
    // 如果当前按的是 'k'，且上一个按的是 'j'
    if (c == 'k' && last_key == 'j') {
        // 计算时间间隔 (毫秒)
        auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_key_time).count();
        
        // 如果间隔小于 500ms，判定为连续按键
        if (diff < 500) {
            // 1. 删除刚才屏幕上已经显示的 'j'
            if (cursor_col > 0) {
                buffer->deleteChar(cursor_row, cursor_col - 1);
                cursor_col--;
            }

            // 2. 退出插入模式 (同 ESC 逻辑)
            mode = MODE_NORMAL;
            status_msg = "";
            // Vim 习惯：退出插入模式时光标左移一格
            if (cursor_col > 0) cursor_col--;
            
            // 3. 重置状态并返回，不输入 'k'
            last_key = 0;
            buffer->setDirty(true);
            return;
        }
    }
    // -------------------------

    // ESC: 退出插入模式 (保持原有逻辑，处理方向键前缀)
    if (c == 27) {
        char seq[3];
        if (read(STDIN_FILENO, &seq[0], 1) != 1) {
            mode = MODE_NORMAL;
            status_msg = "";
            if (cursor_col > 0) cursor_col--;
            last_key = 0; // 重置
            buffer->setDirty(true);
            return;
        }
        if (read(STDIN_FILENO, &seq[1], 1) != 1) return;

        if (seq[0] == '[') {
            switch (seq[1]) {
                case 'A': if (cursor_row > 0) cursor_row--; break; // Up
                case 'B': if (cursor_row < buffer->getLineCount() - 1) cursor_row++; break; // Down
                case 'C': cursor_col++; break; // Right
                case 'D': if (cursor_col > 0) cursor_col--; break; // Left
            }
        }
        last_key = 0; // 方向键打断连续按键序列
        buffer->setDirty(true);
        return;
    }

    // Enter: 换行
    else if (c == 13 || c == 10) {
        buffer->splitLine(cursor_row, cursor_col);
        cursor_row++;
        cursor_col = 0;
        last_key = 0; // 重置
    }
    // Backspace: 删除
    else if (c == 127 || c == 8) {
        if (cursor_col > 0) {
            buffer->deleteChar(cursor_row, cursor_col - 1);
            cursor_col--;
        } else if (cursor_row > 0) {
            int prev_len = buffer->getLineLength(cursor_row - 1);
            buffer->mergeLine(cursor_row);
            cursor_row--;
            cursor_col = prev_len;
        }
        last_key = 0; // 重置
    }
    // 普通字符输入
    else if (!iscntrl(c)) {
        buffer->insertChar(cursor_row, cursor_col, c);
        cursor_col++;
        
        // [关键] 记录这次按键，供下一次检测使用
        last_key = c;
        last_key_time = now;
    }
    else {
        // 其他控制字符，重置记录
        last_key = 0;
    }

    buffer->setDirty(true);
}



void SimpleTextEditor::editorScroll()
{
    // 1. 光标垂直越界检查
    if (cursor_row >= buffer->getLineCount()) {
        cursor_row = buffer->getLineCount() > 0 ? buffer->getLineCount() - 1 : 0;
    }
    if (cursor_row < 0) cursor_row = 0;

    // 2. 光标水平越界检查 (snap to line length)
    int cur_line_len = buffer->getLineLength(cursor_row);
    if (cursor_col > cur_line_len) cursor_col = cur_line_len;
    if (cursor_col < 0) cursor_col = 0;

    // 3. 计算滚动偏移
    int content_rows = current_size.rows - 2; // 除去标题和UI

    // 垂直滚动
    if (cursor_row < row_offset) {
        row_offset = cursor_row;
    }
    if (cursor_row >= row_offset + content_rows) {
        row_offset = cursor_row - content_rows + 1;
    }

    // 水平滚动
    if (cursor_col < col_offset) {
        col_offset = cursor_col;
    }
    if (cursor_col >= col_offset + current_size.cols) {
        col_offset = cursor_col - current_size.cols + 1;
    }
}

void SimpleTextEditor::saveFile() {
    // 假设 TextBuffer 实现了保存逻辑
    // buffer->saveToFile("filename.txt"); 
    // 这里仅做状态演示
    updateStatusMessage("File saved (Demo)");
}

std::string SimpleTextEditor::getFilename() const {
    return "untitled.txt"; // 临时，实际应从 Buffer 获取
}

void SimpleTextEditor::drawStatusBar()
{
    // 倒数第二行
    terminal->moveCursor(current_size.rows - 1, 1);
    
    std::string mode_str = (mode == MODE_NORMAL) ? " NORMAL " : 
                           (mode == MODE_INSERT) ? " INSERT " : 
                                                   " COMMAND";
    
    std::string pos_str = " Ln " + std::to_string(cursor_row + 1) + 
                          ", Col " + std::to_string(cursor_col + 1);
    
    std::string msg = mode_str + " | " + status_msg;
    
    // 截断过长信息
    if ((int)msg.length() > current_size.cols) msg = msg.substr(0, current_size.cols);
    
    // 绘制反色条
    std::cout << "\x1b[7m" << msg; 
    // 填充空白
    for (int i = msg.length(); i < current_size.cols - (int)pos_str.length(); i++) std::cout << " ";
    std::cout << pos_str << "\x1b[0m"; 
}

void SimpleTextEditor::drawCommandLine()
{
    // 最后一行
    // 修正: 删除了这里的 'ß' 字符
    terminal->moveCursor(current_size.rows, 1); 
    std::cout << "\x1b[K"; // 清空整行

    if (mode == MODE_COMMAND) {
        // 黄色显示命令
        std::cout << "\x1b[33m" << commandBuffer << "\x1b[0m"; 
    }
}

void SimpleTextEditor::run()
{
    // 调用基类 run，基类会调用 updateTerminalSize 确保初始尺寸正确
    AbstractEditor::run();  
}












