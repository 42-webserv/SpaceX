#==============================================================================
#	Default Setting
#==============================================================================

NAME		= spacex



# VPATH		:=	$(shell ls -R)
# CONFIG_DEBUG, CONFIG_STATE_DEBUG, SOCKET_DEBUG, LEAK, LOG_MODE
# DEBUG_FLAG	+= SYNTAX_DEBUG
# DEBUG_FLAG	+=  SPACE_RESPONSE_TEST
# DEBUG_FLAG 	+= CONFIG_STATE_DEBUG
# DEBUG_FLAG	+=	LEAK
# DEBUG_FLAG	+= LOG_FILE_MODE
# DEBUG_FLAG	+= LOG_MODE
# DEBUG_FLAG 	+= CONFIG_DEBUG
DEBUG_FLAG	+=	DEBUG

ifdef DEBUG_FLAG
	LOG	+=	$(addprefix -D , $(DEBUG_FLAG))
endif

# CXXFLAGS	+=	$(LOG)
CXXFLAGS	+=	-D CONSOLE_LOG

SRC			=	spacex.cpp \
				spx_autoindex_generator.cpp \
				spx_buffer.cpp \
				spx_cgi_chunked.cpp \
				spx_cgi_module.cpp \
				spx_client.cpp \
				spx_core_util_box.cpp \
				spx_kqueue_module.cpp \
				spx_parse_config.cpp \
				spx_port_info.cpp \
				spx_req_res_field.cpp \
				spx_session_storage.cpp \
				spx_syntax_checker.cpp


SRC_DIR		=	source/
OBJ_DIR		=	object/
INC_DIR		=
OBJ			=	$(addprefix $(OBJ_DIR), $(SRC:.cpp=.o))

#==============================================================================
#	Compile Flags
#==============================================================================
CXX			=	c++
CXX_WARN_FLAGS	=	all extra error
#// NOTE: Need to uncomment later
CXX_STD_FLAGS	=	c++98
CXXFLAGS	+=	$(addprefix -W, $(CXX_WARN_FLAGS))
CXXFLAGS	+= -pedantic
CXXFLAGS	+=	$(addprefix -std=, $(CXX_STD_FLAGS))

RM			=	rm -f

OPT			=	-O3
DEBUG		=	-g
SNTZ		=	-fsanitize=address -fsanitize=undefined -fno-omit-frame-pointer
MEM			=	-fsanitize=memory -fsanitize-memory-track-origins \
				-fPIE -pie -fno-omit-frame-pointer
LEAK		=	-fsanitize=leak

# CXXFLAGS	+=	$(DEBUG)
# CXXFLAGS	+=	$(SNTZ)
# CXXFLAGS	+=	$(OPT)
# CXXFLAGS	+=	-fno-sanitize-recover
# CXXFLAGS	+=	-fstack-protector -D_GLIBCXX_DEBUG -D_GLIBCXX_DEBUG_PEDANTIC

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

spx			: ; make re CXXFLAGS="$(CXXFLAGS) $(LOG) $(DEBUG) $(SNTZ)"

sntz		: ; make re CXXFLAGS="$(CXXFLAGS) $(DEBUG) $(SNTZ)"

log			: ; make re CXXFLAGS="$(CXXFLAGS) $(LOG)"
