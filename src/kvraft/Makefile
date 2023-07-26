-include ../../Global.mk

ROOTDIR = ../..
LIBS    := $(LIBS) -lkvraft-rpc -lraft

KV_SOURCES = KVServer.cpp KVClerk.cpp

KV_OBJECTS = $(addprefix $(OBJECTS_DIR)/, $(KV_SOURCES:cpp=o))
OBJECTS = $(KV_OBJECTS)

DEPENDENCIES = $(LIB_DIR)/libraft.a $(LIB_DIR)/libkvraft-rpc.a

all: $(BIN)/kvraft $(LIB_DIR)/libkvraft.a

$(BIN)/kvraft: $(OBJECTS) $(DEPENDENCIES)
	$(CC) $(CPPFLAGS) $(LIBS) $(LDFLAGS) $(INC) -o $@ $(OBJECTS) KVMain.cpp

$(LIB_DIR)/libkvraft.a: $(OBJECTS) $(DEPENDENCIES)
	$(AR) $@ $(OBJECTS)

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

.PHONY: clean
clean:
	make -C $(SRC_DIR)/raft clean
	rm -rf $(BIN)/kvraft $(OBJECTS) $(BIN)/kvraft $(LIB_DIR)/libkvraft.a