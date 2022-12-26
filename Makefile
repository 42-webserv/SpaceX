#==============================================================================
#	Default Setting
#==============================================================================

NAME		= spacex

# VPATH		:=	$(shell ls -R)
# CONFIG_DEBUG, CONFIG_STATE_DEBUG, SOCKET_DEBUG, LEAK, LOG_MODE
# DEBUG_FLAG	+=	LEAK
DEBUG_FLAG	+=	DEBUG
# DEBUG_FLAG	+=	YOMA_SEARCH_DEBUG
DEBUG_FLAG	+=  SOCKET_DEBUG
# DEBUG_FLAG  +=  SPACE_RESPONSE_TEST
# DEBUG_FLAG 	+= CONFIG_DEBUG
# DEBUG_FLAG 	+= CONFIG_STATE_DEBUG
# DEBUG_FLAG += LOG_MODE

LOG	+=	$(addprefix -D, $(DEBUG_FLAG))
SRC			=	spacex.cpp \
				spx_parse_config.cpp \
				spx_port_info.cpp \
				spx_core_util_box.cpp \
				spx_response_generator.cpp \
				spx_autoindex_generator.cpp \
				spx_syntax_checker.cpp \
				spx_kqueue_main.cpp


SRC_DIR		=	source/
OBJ_DIR		=	object/
INC_DIR		=
OBJ			=	$(addprefix $(OBJ_DIR), $(SRC:.cpp=.o))

#==============================================================================
#	Compile Flags
#==============================================================================
CXX			=	c++
CXX_WARN_FLAGS	=	#all extra error
CXX_STD_FLAGS	=	c++98
CXXFLAGS	+=	$(addprefix -W, $(CXX_WARN_FLAGS))
CXXFLAGS	+=	$(addprefix -std=, $(CXX_STD_FLAGS))
# CXXFLAGS	+=	-fsanitize=address -fsanitize=undefined -fno-omit-frame-pointer
CXXFLAGS	+=	$(LOG)
# CXXFLAGS	+=	-Wc++11-extensions

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
