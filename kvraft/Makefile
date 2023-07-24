-include ../Global.mk

ROOTDIR = ..
LIBS    := $(LIBS) -lraft
RPC     = kvraft-rpc

KV_SOURCES = KVServer.cpp KVMain.cpp

KV_OBJECTS = $(addprefix $(OBJECTS_DIR)/, $(KV_SOURCES:cpp=o))
OBJECTS = $(KV_OBJECTS)

all: make-raft $(BIN)/kvraft

$(BIN)/kvraft: $(KV_OBJECTS) $(LIBDIR)/libraft.a
	$(CC) $(CPPFLAGS) $(LIBS) $(LDFLAGS) $(INC) -o $@ $^

$(RPC): KVRaft.thrift
	rm -rf $@
	thrift -r --gen cpp $<
	mv gen-cpp $(RPC)
	$(CC) $(CPPFLAGS) $(INC) -c $(RPC)/KVRaft.cpp -o $(RPC)/KVRaft.o
	$(CC) $(CPPFLAGS) $(INC) -c $(RPC)/KVRaft_types.cpp -o $(RPC)/KVRaft_types.o
	cp $(RPC)/*.h $(INC_DIR)/kvraft/rpc

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

.PHONY: clean make-raft
clean:
	make -C $(ROOTDIR)/raft clean
	rm -rf $(BIN)/kvraft $(RPC)

make-raft:
	make -C $(ROOTDIR)/raft