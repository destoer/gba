CFLAGS = -O2 -DDEBUG -std=c++17 -Wall -Werror -Wextra -g
TARGET = emu
LDFLAGS = -lSDL2
CC = g++
OBJDIR=obj
DIRS = src src/fmt 

CFILES = $(foreach DIR,$(DIRS),$(wildcard $(DIR)/*.cpp))
COBJFILES = $(CFILES:%.cpp=$(OBJDIR)/%.o)

$(TARGET): $(COBJFILES)
	$(CC) $(CFLAGS) $(COBJFILES) -o $(TARGET) $(LDFLAGS)

$(COBJFILES): $(OBJDIR)/%.o : %.cpp
	$(CC) $(CFLAGS) -c $< -o $@

