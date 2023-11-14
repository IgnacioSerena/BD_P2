CC = gcc -g
CFLAGS = -Wall -Wextra -pedantic -ansi
LDLIBS = -lodbc -lcurses -lpanel -lmenu -lform

# recompile if this header changes
HEADERS = odbc.h bpass.h  lmenu.h search.h windows.h loop.h

EXE = menu
OBJ = $(EXE).o bpass.o odbc.o loop.o  search.o windows.o

all : $(EXE)

# compile all files ending in *.c
%.o: %.c $(HEADERS)
	@echo Compiling $<...
	$(CC) $(CFLAGS) -c -o $@ $<

# link binary
$(EXE): $(DEPS) $(OBJ)
	$(CC) -o $(EXE) $(OBJ) $(LDLIBS)

clean:
	rm -f *.o core $(EXE)

run:
	./$(EXE)

runv:
	valgrind --leak-check=full ./$(EXE) 2> valgrind.txt
