TARGET = a.out

INC =  
ASAN_LIB = -lasan
LIB = 
ASAN_FLAG = -fsanitize=address -fsanitize-recover=all -D__SANITIZE_ADDRESS__=1 -fno-omit-frame-pointer 
FLAG = -g
CC = g++

$(TARGET): *.c* 
	$(CC) $^ $(FLAG) $(INC) $(LIB)


clear:
	rm -f $(TARGET)

PHONY: clear
