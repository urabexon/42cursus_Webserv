PROGRAM := webserv

CXX = c++
CXXFLAGS = -Wall -Wextra -Werror -D_GLIBCXX_USE_CXX11_ABI=0 -std=c++98
DEBUGFLAGS = -g3 -O0 -fsanitize=address 

INCDIR = inc
SRCDIR = src
OBJDIR = obj

SRC = $(wildcard $(SRCDIR)/**/*.cpp) $(wildcard $(SRCDIR)/*.cpp)
OBJ = $(SRC:$(SRCDIR)/%.cpp=$(OBJDIR)/%.o)

all: $(PROGRAM)

$(PROGRAM): $(OBJ)
	$(CXX) $(CXXFLAGS) $(OBJ) -o $@

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -I$(INCDIR) -c $< -o $@

clean:
	rm -rf $(OBJDIR)

fclean: clean
	rm -f $(PROGRAM)

re: fclean all

ifeq ($(MAKECMDGOALS), debug)
CXXFLAGS += $(DEBUGFLAGS)
endif

debug: re

FORMAT_DIRS = src inc
FORMAT_EXTENSIONS = h hpp cpp
FORMAT_FILES = $(shell find $(FORMAT_DIRS) \( -name '*.h' -o -name '*.cpp' \))

fmt:
	clang-format --style=Google -i $(FORMAT_FILES)

.PHONY: all clean fclean re debug fmt
