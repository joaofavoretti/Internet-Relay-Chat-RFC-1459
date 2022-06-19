CC=gcc
SOURCE_FILES=client.c
PROGRAM=client
DEBUG_FLAGS=-D DEBUG -g3
DEFAULT_PORT=47783

compile-debug:
	$(CC) -o $(PROGRAM) $(DEBUG_FLAGS) $(SOURCE_FILES)

compile:
	$(CC) -o $(PROGRAM) $(SOURCE_FILES)

run:
	./$(PROGRAM) localhost $(DEFAULT_PORT)

clean:
	rm -f $(PROGRAM)
