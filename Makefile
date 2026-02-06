CC = clang
CFLAGS = -Wall -luring -O2

EXEC = lndir

SRC = src/main.c src/string_list.c src/lndir_uring.c

# Single compilation unit

$(EXEC):
	 $(CC) $(SRC) $(CFLAGS) -o $@


# Seperate compilation units + linking

# OBJ = $(SRC:.c=.o)

# %.o: %.c
# 	$(CC) -c $< -o $@

# $(EXEC): $(OBJ)
# 	$(CC) $(OBJ) $(CFLAGS) -o $@


clean:
	rm -f $(OBJ) $(EXEC) unity
