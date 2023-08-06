-include ../../Global.mk

ROOTDIR = ../..
LIBS    := $(LIBS) -lkvraft-rpc -lraft -lkvraft

SHARDKV_SRC = ShardCtrler.cpp ShardCtrlerClerk.cpp ShardKV.cpp ShardKVClerk.cpp

SHARDKV_OBJ = $(addprefix $(OBJECTS_DIR)/, $(SHARDKV_SRC:cpp=o))
OBJECTS = $(SHARDKV_OBJ)

DEPENDENCIES = $(LIB_DIR)/libraft.a $(LIB_DIR)/libkvraft-rpc.a $(LIB_DIR)/libkvraft.a 

all: $(LIB_DIR)/libshardkvraft.a

$(LIB_DIR)/libshardkvraft.a: $(OBJECTS) $(DEPENDENCIES)
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
	make -C $(ROOTDIR) clean
	rm -rf $(LIB_DIR)/libshardkvraft.a