TYPE := app
this_dir := $(shell basename $(CURDIR))

TARGET = $(this_dir).$(TYPE)

FLAGS += -g -D__XTEST__ -Wno-deprecated-declarations -Wno-unused-result  
LIBS += 
INC += 

include ../tools/makefiles/$(TYPE).mk
