export PGDATABASE:=flight
export PGUSER :=alumnodb
export PGPASSWORD :=alumnodb
export PGCLIENTENCODING:=UTF8
export PGHOST:=localhost

DBNAME =$(PGDATABASE)
PSQL = psql
CREATEDB = createdb
DROPDB = dropdb --if-exists
PG_DUMP = pg_dump
PG_RESTORE = pg_restore
CC = gcc -g
CFLAGS = -Wall -Wextra -pedantic -ansi
LDLIBS = -lodbc -lcurses -lpanel -lmenu -lform

# recompile if this header changes
HEADERS = odbc.h bpass.h  lmenu.h search.h windows.h loop.h

EXE = menu
OBJ = $(EXE).o bpass.o odbc.o loop.o  search.o windows.o

# compile all files ending in *.c
%.o: %.c $(HEADERS)
	@echo Compiling $<...
	$(CC) $(CFLAGS) -c -o $@ $<

# link binary
$(EXE): $(DEPS) $(OBJ)
	$(CC) -o $(EXE) $(OBJ) $(LDLIBS)

all: dropdb createdb restore $(EXE) 

mycomando:
	@echo aqui va una descripcion
	@cat myfichero.sql | psql mibasededatos
createdb:
	@echo Creando BBDD
	@$(CREATEDB)
dropdb:
	@echo Eliminando BBDD
	@$(DROPDB) $(DBNAME)
	rm -f *.log
dump:
	@echo creando dumpfile
	@$(PG_DUMP) > $(DBNAME).sql
restore:
	@echo restore data base
	@cat $(DBNAME).sql | $(PSQL)  

clean:
	rm -f *.o core $(EXE)

run:
	./$(EXE)

runv:
	valgrind --leak-check=full ./$(EXE) 2> valgrind.txt
