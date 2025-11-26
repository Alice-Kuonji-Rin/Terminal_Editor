#pragma once
#include "../core/AbstractEditor.hpp"
#include <iostream>

class SimpleTextEditor : public AbstractEditor
{
public:
	void drawInterface() override;
	void processInput() override;
	void run() override;
private:
	void editorScroll();

};