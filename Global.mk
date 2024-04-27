CC          = g++
CPPFLAGS    = -std=c++14 -g -g3 -Wall -fsanitize=address -DGLOG_USE_GLOG_EXPORT
# CPPFLAGS    = -std=c++14 -O2 -Wall
SHAREDFLAGS = -fpic -shared -Wno-return-type-c-linkage
LDFLAGS     = -L $(LIB_DIR)
LIBS        = -lthrift -lgflags -lglog -lfmt -lpthread
INC         = -I $(ROOTDIR)/include
AR          = ar -rcs

BIN         = $(ROOTDIR)/bin
LIB_DIR     = $(ROOTDIR)/lib
INC_DIR     = $(ROOTDIR)/include
SRC_DIR     = $(ROOTDIR)/src
OBJECTS_DIR = $(ROOTDIR)/objs