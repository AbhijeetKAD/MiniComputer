CC = gcc
TARGET = simulator
OBJ = main.o compiler.o memory.o processor.o

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $(TARGET)

main.o: main.c compiler.h memory.h processor.h
	$(CC) -c main.c

compiler.o: compiler.c compiler.h
	$(CC) -c compiler.c

memory.o: memory.c memory.h
	$(CC) -c memory.c

processor.o: processor.c processor.h memory.h
	$(CC) -c processor.c

clean:
	rm -f *.o simulator simulator.exe
