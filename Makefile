CC = g++

mkfile_path := $(abspath $(lastword $(MAKEFILE_LIST)))
mkfile_dir := $(dir $(mkfile_path))

# Directories and settings that can be modified

RM = del /s /q

# End of directories and settings that can be modified

ADDITIONAL_INCLUDE_DIRS = -I $(mkfile_dir)/ChessGM

CFLAGS = -Wall -Wno-class-memaccess -Ofast -std=c++20 $(ADDITIONAL_INCLUDE_DIRS)
LDFLAGS = -static-libstdc++

SRCS := $(wildcard *.cpp) $(wildcard */*.cpp) $(wildcard */*/*.cpp) $(wildcard */*/*/*.cpp)
OBJS := $(SRCS:%.cpp=%.o)

.PHONY: all clean

build: all clean

rebuild: clean build

all: $(OBJS)
	$(CC) -o ChessGM.exe $(OBJS) $(LDFLAGS)
	
%.o: %.cpp
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	$(RM) *.o