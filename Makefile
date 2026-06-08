CXXFLAGS = -std=c++23 -Wall -Wextra -pedantic -Wno-missing-field-initializers -g

BUILD_DIR=build/

RAYLIB_PATH=raylib/raylib-6.0_linux_amd64/
LINK=-L$(RAYLIB_PATH)lib/ -l:libraylib.a -lX11 
INCLUDE=-I$(RAYLIB_PATH)include/

.PHONY: all
all: | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(INCLUDE) -o $(BUILD_DIR)game_of_life main.cpp $(LINK)

$(BUILD_DIR):
	-mkdir -p $(BUILD_DIR)