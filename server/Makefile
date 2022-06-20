CC=gcc
SOURCE_FILES=server.c utils.c commands.c connection.c ../shared/logger.c
PROGRAM=server
DEBUG_FLAGS=-D DEBUG -g3
DEFAULT_PORT=47783


compile:
	$(CC) -o $(PROGRAM) $(SOURCE_FILES)

compile-debug:
	$(CC) -o $(PROGRAM) $(DEBUG_FLAGS) $(SOURCE_FILES)

run:
	./$(PROGRAM) $(DEFAULT_PORT)

clean:
	rm -f $(PROGRAM)