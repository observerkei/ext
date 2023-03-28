MAKEFILE_DIR = $(abspath $(lastword $(MAKEFILE_LIST)))

export PROJECT := $(shell echo $(MAKEFILE_DIR) | awk -F '/' '{printf $$(NF-3)}' )


export BASEDIR = $(shell __tmpstring=`pwd`; echo $${__tmpstring%%$(PROJECT)*}$(PROJECT))
export OUTDIR := $(BASEDIR)/_out

export DESTDIR ?= /usr/
export CFILE ?= *.C
export CFILES ?= $(wildcard *.c)
export OBJS ?= $(CFILES:.c=.o)

export GCOV_FILE ?= $(OBHS:.c=.gcno) $(CFILE:.C=.gcno)

export FLAGS += 
export LIBS += -L/usr/lib/
export INC += -I$(BASEDIR)/
