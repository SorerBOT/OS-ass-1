# Makefile to compile and run file_sync.c with C99 standard

CC = gcc
CFLAGS = -Wall -Werror -g

# Define the target executable
TARGET = file_sync

# Source and object files
SRC = file_sync.c
OBJ = $(SRC:.c=.o)

# Default rule to build the target
all: $(TARGET)

# Rule to create the executable
$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $(TARGET)

# Rule to compile the .c file into .o (object file)
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean up object files and executable
clean:
	rm -f $(OBJ) $(TARGET)

# Rule to run the program
run: $(TARGET)
	./$(TARGET) /path/to/source /path/to/destination

# .PHONY to mark targets that are not real files
.PHONY: all clean run

