CC = clang
CFLAGS = -Wall -luring -O2

BUILD_DIR = bin/
EXEC = $(BUILD_DIR)lndir

SRC = src/main.c src/string_list.c src/lndir_uring.c

all: $(EXEC)


## Single compilation unit

$(EXEC): $(BUILD_DIR)
	 $(CC) $(SRC) $(CFLAGS) -o $@


## Seperate compilation units + linking

# OBJ = $(SRC:.c=.o)

# %.o: %.c
# 	$(CC) -c $< -o $@

# $(EXEC): $(OBJ)
# 	$(CC) $(OBJ) $(CFLAGS) -o $@


$(BUILD_DIR):
	mkdir $(BUILD_DIR)

clean:
	rm -rf $(OBJ) $(EXEC) $(BUILD_DIR)
