VERSION = $(shell grep "^ *\.version = \"" build.zig.zon | sed 's/^.*\.version = //' | sed 's/,.*//')

CC = cc
CFLAGS = -Wall -luring -O2 -DVERSION=\"$(VERSION)\"

EXEC = lndir
SRC = src/main.c src/string_list.c src/lndir.c src/dir_walker.c
VERSION_FILE = build.zig.zon
MAKEFILE = Makefile

all: $(EXEC)

## Single compilation unit

$(EXEC): $(SRC) $(VERSION_FILE) $(MAKEFILE)
	 $(CC) $(SRC) $(CFLAGS) -o $@


## Seperate compilation units + linking
## A single CU compiles fast enough for now

# OBJ = $(SRC:.c=.o)

# %.o: %.c
# 	$(CC) -c $< -o $@

# $(EXEC): $(OBJ) $(VERSION_FILE) $(MAKEFILE)
# 	$(CC) $(OBJ) $(CFLAGS) -o $@


clean:
	rm -rf $(OBJ) $(EXEC) $(BUILD_DIR)

clean_zig:
	rm -rf $(OBJ) $(EXEC) $(BUILD_DIR) .zig-cache/ zig-out/
