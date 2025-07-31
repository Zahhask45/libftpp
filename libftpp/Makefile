NAME		= libftpp.a
LIBCPP		= ar rc
CXX			= c++
RM			= rm -f
CXXFLAGS	= -Wall -Wextra -Werror -pedantic -std=c++11

INC_DIRS	= -I ./inc/ \
					-I ./data_struct/inc/ \
					-I ./design_patterns/inc/ \
					-I ./network/inc/ \
					-I ./thread/inc/

SRC_DIR		= ./data_struct/src/ \
					./design_patterns/src/ \
					./network/src/ \
					./thread/src/

# DATA_DIR	= data_struct/src/
# DESIGN_DIR	= design_patterns/src/
# NET_DIR		= network/src/
# THREAD_DIR	= thread/src/
# 
# DATA		= data_struct.cpp
# 
# DATA_SRC	= $(addprefix $(DATA_DIR), $(DATA))


OBJ_DIR		= ./obj/



# SRC			= $(DATA_SRC)
SRCS		= $(shell find $(SRC_DIR) -name '*.cpp')
OBJS		= $(patsubst %.cpp, $(OBJ_DIR)%.o, $(SRCS))
# OBJ			= $(addprefix $(OBJ_DIR)/, $(SRC:.cpp=.o))


all: ${NAME}

${NAME}: ${OBJS}
	${LIBCPP} ${NAME} ${OBJS}


$(OBJ_DIR)%.o: %.cpp
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@ $(INC_DIRS)


clean:
	${RM} ${OBJ}

fclean: clean
	${RM} ${NAME}

re: fclean all
