MKDIR_P=mkdir -p

SERVER_SOURCE=*.cpp
SERVER_OUTPUT_DIR=output
SERVER_OUTPUT=$(SERVER_OUTPUT_DIR)/network_text_editor_server
SERVER_LIBRARIES=

INCLUDES=

CXX=g++
CXXFLAGS=-Wall -ansi -pedantic -std=c++11

all: $(SERVER_OUTPUT)

$(SERVER_OUTPUT): $(SERVER_SOURCE)
	$(MKDIR_P) $(SERVER_OUTPUT_DIR)
	$(CXX) $(CXXFLAGS) $(SERVER_SOURCE) -o$(SERVER_OUTPUT)

clean:
	rm -f $(SERVER_OUTPUT)
