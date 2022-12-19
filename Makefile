#==============================================================================
#	Default Setting
#==============================================================================

NAME		= spacex

# VPATH		:=	$(shell ls -R)
# CONFIG_DEBUG, CONFIG_STATE_DEBUG, SOCKET_DEBUG, LEAK
DEBUG_FLAG	+=	DEBUG
# DEBUG_FLAG	+=  SOCKET_DEBUG
# DEBUG_FLAG 	+= CONFIG_STATE_DEBUG
# DEBUG_FLAG 	+= CONFIG_DEBUG

LOG	+=	$(addprefix -D, $(DEBUG_FLAG))
SRC			=	spacex.cpp \
				spx_config_parse.cpp \
				spx_config_port_info.cpp \
				spx_socket_init.cpp \
				spx_syntax_checker.cpp \
				spx_path_resolver.cpp


SRC_DIR		=	source/
OBJ_DIR		=	object/
INC_DIR		=	include/
OBJ			=	$(addprefix $(OBJ_DIR), $(SRC:.cpp=.o))

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
		$(CXX) $(CXXFLAGS) -I $(INC_DIR) -o $@ -c $<
