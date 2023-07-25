-include ../../Global.mk

ROOTDIR = ../..
LIBS    := $(LIBS) -lkvraft-rpc -lraft

KV_SOURCES = KVServer.cpp KVMain.cpp

KV_OBJECTS = $(addprefix $(OBJECTS_DIR)/, $(KV_SOURCES:cpp=o))
OBJECTS = $(KV_OBJECTS)

all: make-raft $(BIN)/kvraft $(LIB_DIR)/libkvraft.a

$(BIN)/kvraft: $(OBJECTS)
	$(CC) $(CPPFLAGS) $(LIBS) $(LDFLAGS) $(INC) -o $@ $(OBJECTS)

$(LIB_DIR)/libkvraft.a:
	$(AR) $@ $(OBJECTS)

$(LIB_DIR)/libraft.a:
	make -C $(SRC_DIR)/raft

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
	make -C $(SRC_DIR)/raft clean
	rm -rf $(BIN)/kvraft $(RPC)

make-raft:
	make -C $(SRC_DIR)/raft