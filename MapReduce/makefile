CC          = g++
CPPFLAGS    = -std=c++14 -g -Wall -fsanitize=address
SHAREDFLAGS = -fpic -shared -Wno-return-type-c-linkage
LDFLAGS     = -L $(SRC)/gen-cpp
LIBS        = -lthrift -lmaster_rpc -lglog -lfmt
INC         =
AR          = ar -rcs

SRC         = src
BIN         = bin

RPC_LIB = $(SRC)/gen-cpp/libmaster_rpc.a

all: $(BIN)/master $(BIN)/worker user-program/WordCount.so

$(BIN)/%: $(SRC)/%.cpp $(RPC_LIB) src/tools.h
	$(CC) $(CPPFLAGS) $(LIBS) $(LDFLAGS) $(INC) -o $@ $<

user-program/%.so : user-program/%.cpp
	$(CC) $(CPPFLAGS) $(SHAREDFLAGS) $(INC) -o $@ $<

$(RPC_LIB): $(SRC)/gen-cpp
	cd $< ; \
	$(CC) $(CPPFLAGS) $(INC) -c Master.cpp -o Master.o; \
	$(CC) $(CPPFLAGS) $(INC) -c master_types.cpp -o master_types.o;\
	$(AR) libmaster_rpc.a  Master.o master_types.o; \

$(SRC)/gen-cpp: $(SRC)/master.thrift
	rm -rf $@
	thrift -r --gen cpp $<
	mv gen-cpp $(SRC)

.PHONY: clean all
clean:
	rm -rf $(SRC)/gen-cpp $(BIN)/* user-program/*.so