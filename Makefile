#==============================================================================
#	Default Setting
#==============================================================================

NAME		= spacex

# VPATH		:=	$(shell ls -R)

LOG			=	-D DEBUG -D SOCKET_DEBUG

SRC			=	spacex.cpp \
				spx_config_parse.cpp \
				spx_config_port_info.cpp \
				spx_socket_init.cpp \
				spx_syntax_chunked.cpp \
				spx_syntax_request.cpp


SRC_DIR		=	core/
OBJ_DIR		=	obj/
OBJ			=	$(addprefix $(OBJ_DIR), $(SRC:.cpp=.o))
INC_DIR		=	core/
LIB_LNK		=	-I $(INC_DIR)

#==============================================================================
#	Compile Flags
#==============================================================================
CXX			=	c++
CXX_WARN_FLAGS	=	all extra error
CXX_STD_FLAGS	=	c++98
CXXFLAGS	+=	$(addprefix -W, $(CXX_WARN_FLAGS))
CXXFLAGS	+=	$(addprefix -std=, $(CXX_STD_FLAGS))
# CXXFLAGS	+=	-fsanitize=address -fsanitize=undefined -fno-omit-frame-pointer
CXXFLAGS	+=	$(LOG)

RM			=	rm -f

DEBUG		=	-g
SNTZ		=	-fsanitize=address -fsanitize=undefined -fno-omit-frame-pointer
MEM			=	-fsanitize=memory -fsanitize-memory-track-origins \
				-fPIE -pie -fno-omit-frame-pointer
LEAK		=	-fsanitize=leak

#==============================================================================
#	Make Part
#==============================================================================
.PHONY		:	all clean fclean re
all			:	$(NAME)
clean		:	; $(RM) -r $(OBJ_DIR)
fclean		:	clean ; $(RM) $(NAME)
re			:
		make fclean
		make all -j 8

$(NAME)		:	$(OBJ)
		$(CXX) $(CXXFLAGS) $^ -o $@

$(OBJ_DIR)	:
		mkdir -p $(OBJ_DIR)

$(OBJ)		:	| $(OBJ_DIR)

$(OBJ_DIR)%.o:	$(SRC_DIR)%.cpp $(INC_DIR)
		$(CXX) $(CXXFLAGS) -o $@ -c $<
