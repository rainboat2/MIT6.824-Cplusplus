ROOTDIR     = ..
-include ../Global.mk
RPC         = raft-rpc

RAFT_SOURCES = raft.cpp ClientManager.cpp Persister.cpp

RAFT_OBJECTS = $(addprefix $(OBJECTS_DIR)/, $(RAFT_SOURCES:cpp=o))

OBJECTS = $(RAFT_OBJECTS)

RAFT_LIB = $(LIBDIR)/libraft.a

all: $(RAFT_LIB) $(BIN)/raft_main

$(BIN)/raft_main: raft_main.cpp $(RAFT_LIB)
	$(CC) $(CPPFLAGS) $(LIBS) $(LDFLAGS) -lraft $(INC) -o $@ $<

$(RAFT_LIB): $(RPC) $(OBJECTS)
	$(AR) $@ $(RAFT_OBJECTS) $(RPC)/RaftRPC.o $(RPC)/raft_types.o

$(RPC): raft.thrift
	rm -rf $@
	thrift -r --gen cpp $<
	mv gen-cpp $(RPC)
	$(CC) $(CPPFLAGS) $(INC) -c $(RPC)/RaftRPC.cpp -o $(RPC)/RaftRPC.o
	$(CC) $(CPPFLAGS) $(INC) -c $(RPC)/raft_types.cpp -o $(RPC)/raft_types.o
	ln -f $(RPC)/raft_types.h $(ROOTDIR)/include/raft/rpc/raft_types.h
	ln -f $(RPC)/RaftRPC.h $(ROOTDIR)/include/raft/rpc/RaftRPC.h

# generate object file
-include $(OBJECTS:.o=.d)
	
$(OBJECTS_DIR)/%.d: %.cpp
	@set -e; \
	mkdir -p $(OBJECTS_DIR); \
	rm -f $@; \
	$(CC) $(CPPFLAGS) $(INC) -MM -MT $(@:.d=.o) $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

$(OBJECTS): %.o:
	$(CC) $(CPPFLAGS) $(INC) -c $< -o $@

.PHONY: clean all
clean:
	rm -rf raft-rpc $(RAFT_LIB) $(RPC_LIB) $(OBJECTS) $(OBJECTS:.o=.d)