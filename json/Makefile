TARGET = a.out

INC =  
ASAN_LIB = -lasan
LIB = 
ASAN_FLAG = -fsanitize=address -fsanitize-recover=all -D__SANITIZE_ADDRESS__=1 -fno-omit-frame-pointer 
FLAG = -g -D__XTEST__
CC = g++

$(TARGET): *.c* 
	$(CC) $^ $(FLAG) $(INC) $(LIB)


clear:
	rm -f $(TARGET)

asan: *.c*
	$(CC) $^ $(FLAG) $(INC) $(LIB) $(ASAN_LIB) $(ASAN_FLAG)

valgrind: $(TARGET)
	valgrind --leak-check=full --show-reachable=yes --trace-children=yes ./$(TARGET)

PHONY: clear asan valgrind
