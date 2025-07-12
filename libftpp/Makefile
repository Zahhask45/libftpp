NAME		= libftpp.a
LIBCPP		= ar rc
CXX			= c++
RM			= rm -f
CXXFLAGS	= -Wall -Wextra -Werror -pedanctic -std=c++20

INC_DIR		= -I ./inc/ \
					-I ./data_struct/inc/ \
					-I ./design_patterns/inc/ \
					-I ./network/inc/ \
					-I ./thread/inc/

SRC_DIR		= ./data_struct/src/ \
					./design_patterns/src/ \
					./network/src/ \
					./thread/src/


SRCS		= 
SRC			= $(addprefix $(SRC_DIR), $(SRCS))
OBJS		= $(SRCS:.cpp=.o)
OBJ			= $(addprefix $(OBJ_DIR), $(OBJS))


all: ${NAME}

.c.o:
	${CXX} ${CXXFLAGS} -c ${SRC}

${NAME}: ${OBJS}
	${LIBCPP} ${NAME} ${OBJS}


clean:
	${RM} ${OBJS}

fclean: clean
	${RM} ${NAME}

re: fclean all
