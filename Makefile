CXX = clang++
CXXFLAGS = -std=c++17 -Wall -Wextra -I.
TARGET = build/editor

SOURCES = \
    core/AbstractEditor.cpp \
    core/TextBuffer.cpp \
    core/TerminalController.cpp \
    editors/SimpleTextEditor.cpp \
    features/CommandShell.cpp\
    main.cpp

OBJECTS = $(SOURCES:%.cpp=build/%.o)

all: $(TARGET)

$(TARGET): $(OBJECTS)
	@mkdir -p $(dir $@)
	$(CXX) $(OBJECTS) -o $(TARGET)
	@echo "编译完成！可执行文件: $(TARGET)"

build/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf build

run: $(TARGET)
	./$(TARGET)

.PHONY: all clean run