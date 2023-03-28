MYDIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
include $(MYDIR)/env.mk

all: $(SUBDIRS:=.all)

clean: $(SUBDIRS:=.clean)

unittest: $(SUBDIRS:=.unittest)

install: $(SUBDIRS:=.install)

%.all:
	cd $* && $(MAKE)

%.clean:
	cd $* && $(MAKE) clean

%.unittest:
	cd $* && $(MAKE) unittest

%.install: $(SUBDIRS:=.install)
	cd $* && $(MAKE) install

.PHONY: all clean install unittest

