NAME		= libftpp.a
LIBCPP		= ar rc
CXX			= c++
RM			= rm -rf
CXXFLAGS	= -Wall -Wextra -Werror -pedantic -std=c++11

INC			= -I ./inc/

SRC_DIR		= ./src/


OBJ_DIR		= ./obj/

SRCS		= data_buffer.cpp \
				memento.cpp \
				thread_safe_iostream.cpp \
				thread.cpp

SRC			= $(addpredix $(SRC_DIR), $(SRCS))
OBJS		= $(SRCS:.cpp=.o)
OBJ			= $(addprefix $(OBJ_DIR), $(OBJS))


all: ${NAME}

${NAME}: ${OBJ}
	${LIBCPP} ${NAME} ${OBJ}

$(OBJ): | $(OBJ_DIR)
$(OBJ_DIR)%.o: $(SRC_DIR)%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@ $(INC)

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)


clean:
	${RM} ${OBJ_DIR}

fclean: clean
	${RM} ${NAME}

re: fclean all
