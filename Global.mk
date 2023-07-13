CC          = g++
# CPPFLAGS    = -std=c++14 -g -g3 -Wall -fsanitize=address
CPPFLAGS    = -std=c++14 -O2 -Wall
SHAREDFLAGS = -fpic -shared -Wno-return-type-c-linkage
LDFLAGS     = -L $(LIBDIR)
LIBS        = -lthrift -lgflags -lglog -lfmt
INC         = -I $(ROOTDIR)/include
AR          = ar -rcs

BIN         = $(ROOTDIR)/bin
LIBDIR      = $(ROOTDIR)/lib
OBJECTS_DIR = $(ROOTDIR)/objs