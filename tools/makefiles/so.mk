MYDIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
include $(MYDIR)/env.mk

TYPE ?= so

C_OBJS = $(wildcard *.c)
CPP_OBJS = $(wildcard *.cpp)
OBJS = $(C_OBJS) $(CPP_OBJS)

SUBDIRS ?=
INSTALL_DIR ?= $(LIBDIR)
OUTDIR := $(BASEDIR)/_out/lib
DESTDIR := /usr/lib

ifeq ($(findstring .cpp, $(OBJS)),.cpp)

CC = g++

else##($(findstring .c, $(OBJS)),)

CC = gcc

endif##($(findstring .c, $(OBJS)),)


%.o:%.c
	$(CC) -c -o $@ $^ $(FLAGS) $(INC)

%.o:%.cpp
	$(CC) -c -o $@ $^ $(FLAGS) $(INC)

$(TARGET): $(C_OBJS:.c=.o) $(CPP_OBJS:.cpp=.o)
	$(CC) -o $@ $^ $(FLAGS) $(LIBS)
	if [ ! -d $(OUTDIR) ]; then install -d $(OUTDIR); fi
	cp $@ $(OUTDIR)/

all: $(TARGET)

clean:
	rm -r $(TARGET) $(OUTDIR) *.o
