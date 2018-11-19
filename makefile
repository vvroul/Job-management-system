OBJS 	= Console.o Coord.o Utilities.o
SOURCE  = Console.cpp Coord.cpp Utilities.cpp
HEADER  =  Headers.hpp Utilities.hpp PoolNode.hpp
CC	= g++
FLAGS   = -g -lrt -c

all : jms_console jms_coord

jms_console : Console.cpp Utilities.cpp
	$(CC) -o $@ $^

jms_coord : Coord.cpp Utilities.cpp
	$(CC) -o $@ $^

Console.o: Console.cpp
	$(CC) $(FLAGS) Console.cpp

Coord.o: Coord.cpp
	$(CC) $(FLAGS) Coord.cpp	

Utilities.o: Utilities.cpp
	$(CC) $(FLAGS) Utilities.cpp
	

# clean
clean:
	rm -f $(OBJS) jms_console jms_coord

#accounting
count:
	wc $(SOURCE) $(HEADER)
