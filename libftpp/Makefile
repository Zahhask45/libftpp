NAME		= libftpp.a
LIBCPP		= ar rc
CXX			= c++
RM			= rm -rf
CXXFLAGS	= -Wall -Wextra -Werror -pedantic -std=c++11 -g

INC			= -I ./inc/

SRC_DIR		= ./src/


OBJ_DIR		= ./obj/

SRCS		= data_buffer.cpp \
				memento.cpp \
				thread_safe_iostream.cpp \
				thread.cpp \
				worker_pool.cpp \
				persistent_worker.cpp \
				message.cpp \
				client.cpp \
				server.cpp \
				ivector2.cpp \
				ivector3.cpp \
				random_2D_coordinate_generator.cpp \
				perlin_noise_2D.cpp

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
	${RM} a.out

fclean: clean
	${RM} ${NAME}

re: fclean all
