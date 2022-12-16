NAME	= a.out
SRC		=
LOG		=

SRC_DIR		=
OBJ_DIR		=	obj/
OBJ			=	$(addprefix $(OBJ_DIR), $(SRC:.cpp=.o))
INC_DIR		=
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
		make all

$(NAME)		:	$(OBJ)
		$(CXX) $(CXXFLAGS) $^ -o $@

$(OBJ_DIR)	:
		mkdir -p $(OBJ_DIR)

$(OBJ)		:	| $(OBJ_DIR)

$(OBJ_DIR)%.o:	$(SRC_DIR)%.cpp $(INC_DIR)
		$(CXX) $(CXXFLAGS) -o $@ -c $<
