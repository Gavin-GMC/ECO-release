# 设置操作系统变量
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
OS := Linux
endif
ifeq ($(OS),)
OS := Windows
endif

# 设置编译器
ifeq ($(OS),Windows)
CC := g++
CXXFLAGS := -std=c++11
else
CC := g++
CXXFLAGS := -std=c++11
endif

# 设置路径分隔符
ifeq ($(OS),Windows)
PATH_SEP := \\
else
PATH_SEP := /
endif

# 设置源文件和目标文件路径
# 工作路径现在是 ECFC 目录
ECFC_RELS_DIR := ./_rels
NUMERICAL_DIR := ./_pdata$(PATH_SEP)numerical
CEC2013_L_DIR := $(NUMERICAL_DIR)$(PATH_SEP)CEC2013_L
CEC2013_N_DIR := $(NUMERICAL_DIR)$(PATH_SEP)CEC2013_N

# 源文件
MAIN_SRC := main.cpp
ALGLIB_SRCS := $(ECFC_RELS_DIR)$(PATH_SEP)alglib$(PATH_SEP)hungarian.cpp
CEC2013_L_SRCS := $(CEC2013_L_DIR)$(PATH_SEP)Benchmarks.cpp $(CEC2013_L_DIR)$(PATH_SEP)F1.cpp $(CEC2013_L_DIR)$(PATH_SEP)F2.cpp $(CEC2013_L_DIR)$(PATH_SEP)F3.cpp $(CEC2013_L_DIR)$(PATH_SEP)F4.cpp $(CEC2013_L_DIR)$(PATH_SEP)F5.cpp $(CEC2013_L_DIR)$(PATH_SEP)F6.cpp $(CEC2013_L_DIR)$(PATH_SEP)F7.cpp $(CEC2013_L_DIR)$(PATH_SEP)F8.cpp $(CEC2013_L_DIR)$(PATH_SEP)F9.cpp $(CEC2013_L_DIR)$(PATH_SEP)F10.cpp $(CEC2013_L_DIR)$(PATH_SEP)F11.cpp $(CEC2013_L_DIR)$(PATH_SEP)F12.cpp $(CEC2013_L_DIR)$(PATH_SEP)F13.cpp $(CEC2013_L_DIR)$(PATH_SEP)F14.cpp $(CEC2013_L_DIR)$(PATH_SEP)F15.cpp
CEC2013_N_SRCS := $(CEC2013_N_DIR)$(PATH_SEP)cec2013.cpp $(CEC2013_N_DIR)$(PATH_SEP)cfunction.cpp $(CEC2013_N_DIR)$(PATH_SEP)rand2.cpp

# 头文件目录
INCLUDE_DIRS := -I. -I$(ECFC_RELS_DIR)$(PATH_SEP)alglib -I$(ECFC_RELS_DIR)$(PATH_SEP)metriclib -I$(CEC2013_L_DIR) -I$(CEC2013_N_DIR)

# 目标文件
TARGET := main

# 默认目标
all: $(TARGET)

# 编译规则
$(TARGET): $(MAIN_SRC) $(ALGLIB_SRCS) $(CEC2013_L_SRCS) $(CEC2013_N_SRCS)
	$(CC) $(CXXFLAGS) $(INCLUDE_DIRS) -o $@ $^

# 清理目标
clean:
	rm -f $(TARGET) *.o

# 帮助目标
help:
	@echo "Makefile targets:"
	@echo "  all       - Build the main executable"
	@echo "  clean     - Remove all generated files"
	@echo "  help      - Display this help message"