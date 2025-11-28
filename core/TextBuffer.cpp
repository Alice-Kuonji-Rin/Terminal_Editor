#include "TextBuffer.hpp"

TextBuffer::TextBuffer() : dirty(true)
{
	insertLine(0, "");
	//insertLine(1, "This is a simple text editor demonstration");
	//insertLine(2, "Use arrow keys to move cursor");
	//insertLine(3, "Type 'q' to quit (if empty), Enter/Backspace work");
	//insertLine(4, "");
}


void TextBuffer::insertLine(int row, const std::string& text)
{
	if(row < 0 || row > static_cast<int>(content.size())) return;
	content.insert(content.begin() + row, {text});
	dirty = true;
}

void TextBuffer::insertChar(int row, int col, char c)
{
	if(row < 0 || row >= static_cast<int>(content.size())) return;
	if(col < 0 || col > content[row].chars.length()) return;
	content[row].chars.insert(col, 1, c);
	dirty = true;
}

void TextBuffer::deleteChar(int row, int col) 
{
    if (row < 0 || row >= static_cast<int>(content.size())) return;
    if (col < 0 || col >= content[row].chars.length()) return;
    content[row].chars.erase(col, 1);
    dirty = true;
}

void TextBuffer::splitLine(int row, int col) 
{
    if (row < 0 || row >= static_cast<int>(content.size())) return;
    std::string current = content[row].chars;
    if (col > static_cast<int>(current.length())) col = current.length();
    
    std::string next_line_content = current.substr(col);
    content[row].chars = current.substr(0, col);
    
    insertLine(row + 1, next_line_content);
}

void TextBuffer::mergeLine(int row) 
{
    if (row <= 0 || row >= static_cast<int>(content.size())) return;
    std::string current = content[row].chars;
    content[row - 1].chars += current;
    content.erase(content.begin() + row);
    dirty = true;
}

std::string TextBuffer::getLine(int row) const 
{
    if (row < 0 || row >= static_cast<int>(content.size())) return "";
    return content[row].chars;
}

int TextBuffer::getLineCount() const 
{
    return static_cast<int>(content.size());
}

int TextBuffer::getLineLength(int row) const 
{
    if (row < 0 || row >= static_cast<int>(content.size())) return 0;
    return static_cast<int>(content[row].chars.length());
}

void TextBuffer::saveToFile()
{
    if(filename.empty()) return;   //没有文件名则不保存

    std::ofstream file(filename);
    if(file.is_open())
    {
        for(const auto& row : content)
        {
            file << row.chars << "\n";
        }
        file.close();
        dirty = true;   //触发刷新以更新状态栏
    }
}


std::string TextBuffer::getFilename() const
{
    return filename;
}
