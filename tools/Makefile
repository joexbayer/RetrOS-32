

SRC_FILES = $(wildcard *.c)
OBJ_FILES = $(SRC_FILES:%.c=$(OUTPUTDIR)%.o)
OUTPUTDIR = ./bin/

all: $(OBJ_FILES)

$(OUTPUTDIR)%.o: %.c
	@mkdir -p $(OUTPUTDIR)
	gcc -c $< -o $@