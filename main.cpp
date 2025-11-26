#include <iostream>
#include "editors/SimpleTextEditor.hpp"

int main()
{
	try{
		SimpleTextEditor editor;
		editor.run();
	} catch (const std::exception& e) {
		std::cerr << "Error: " << e.what() << std::endl;
		return 1;
	}
	return 0;
}