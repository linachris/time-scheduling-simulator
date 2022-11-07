# Paths
INCLUDE = ./include
SRC = ./src

# Compile Options
CC = gcc
CFLAGS = -Wall -Wextra -Werror -g -I$(INCLUDE)
ARGS = 0.5 0.1 0.2 10 40 3

# Objects
OBJS = $(SRC)/ADTPriorityQueue.o $(SRC)/ADTVector.o $(SRC)/semaphore.o $(SRC)/simulator.o 

# Executable file names
EXEC = simulator

# Build executables
$(EXEC): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(EXEC) -lm

run: $(EXEC)
	./$(EXEC) $(ARGS)

valgrind: $(EXEC)
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./$(EXEC) $(ARGS)

# Delete executable, object and .log files
clean:
	rm -f $(EXEC)
	rm -rf $(OBJS)
	rm -f running_state.log