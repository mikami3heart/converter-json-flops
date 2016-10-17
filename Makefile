CXX			= g++
PROJECT		= SurveyFlops
TARGET		= ./bin/$(PROJECT)
#CXXFLAGS	= -Wall -O2 -pipe
CXXFLAGS	= 
SRC_DIR		= ./src
OBJ_DIR		= ./Tmp/obj
EXTLIB_DIR	= ./ext
SRCS		= $(wildcard $(SRC_DIR)/*.cpp)
INCLUDES	= -I$(EXTLIB_DIR)/picojson
OBJS		= $(addprefix $(OBJ_DIR)/, $(notdir $(SRCS:.cpp=.o)))

$(TARGET): $(OBJS)
	mkdir -p ./bin
	$(CXX) -o $@ $(OBJS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	mkdir -p $(OBJ_DIR)
	$(CXX) $(INCLUDES) -o $@ -c $< 

all: clean
	$(TARGET)

clean:
	rm -f $(TARGET) $(OBJS)

