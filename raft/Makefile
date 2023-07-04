CC          = g++
# CPPFLAGS    = -std=c++14 -g -Wall -fsanitize=address
CPPFLAGS    = -std=c++14 -O2 -Wall
SHAREDFLAGS = -fpic -shared -Wno-return-type-c-linkage
LDFLAGS     = -L $(SRC)/gen-cpp
LIBS        = -lthrift -lraft_rpc -lgflags -lglog -lfmt
INC         =
AR          = ar -rcs

SRC         = src
BIN         = bin

RPC_LIB = $(SRC)/gen-cpp/libraft_rpc.a

all: $(BIN)/raft

$(BIN)/%: $(SRC)/%.cpp $(RPC_LIB)
	$(CC) $(CPPFLAGS) $(LIBS) $(LDFLAGS) $(INC) -o $@ $<

$(RPC_LIB): $(SRC)/gen-cpp
	cd $< ; \
	$(CC) $(CPPFLAGS) $(INC) -c RaftRPC.cpp -o RaftRPC.o; \
	$(CC) $(CPPFLAGS) $(INC) -c raft_types.cpp -o raft_types.o;\
	$(AR) libraft_rpc.a  RaftRPC.o raft_types.o; \

$(SRC)/gen-cpp: $(SRC)/raft.thrift
	rm -rf $@
	thrift -r --gen cpp $<
	mv gen-cpp $(SRC)

.PHONY: clean all
clean:
	rm -rf $(SRC)/gen-cpp $(BIN)/* 