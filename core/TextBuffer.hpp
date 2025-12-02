
#pragma once
#include <vector>
#include <string>
#include <fstream>

struct EditorRow	//简单定义EditorRow，方便未来扩展如：增加高亮属性
{
	std::string chars;
};

 
class TextBuffer
{
public:
	TextBuffer();

	void insertChar(int row, int col, char c);
	void deleteChar(int row, int col);
	void insertLine(int row, const std::string& text);
	void splitLine(int row, int col);
	void mergeLine(int row);

	std::string getLine(int row) const;
	int getLineCount() const;
	int getLineLength(int row) const;
	std::string getFilename() const;

	bool isDirty() const { return dirty;}
	void setDirty(bool d) { dirty = d;}

	void saveToFile();
	void setFilename(const std::string& name) { filename = name; }




private:
	std::vector<EditorRow> content;
	bool dirty;
	std::string filename;
};
