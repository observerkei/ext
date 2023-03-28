MYDIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
include $(MYDIR)/env.mk

TYPE ?= app
INSTALL_DIR ?= bin

SUBDIRS ?=

C_OBJS = $(wildcard *.c)
CPP_OBJS = $(wildcard *.cpp)
OBJS = $(C_OBJS) $(CPP_OBJS)

OUTDIR := $(BASEDIR)/_out/bin
DESTDIR := /usr/bin

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
	mv $@ $(OUTDIR)/

all: $(TARGET)

clean:
	if [ -f ${OUTDIR}/$(TARGET) ]; then rm -r $(OUTDIR)/$(TARGET) *.o; fi

