include ../../Global.mk

ROOTDIR = ../..
LIBS   := -lshardkvraft -lraft -lkvraft -lkvraft-rpc -lgtest -lgtest_main -lpthread $(LIBS) 

DEPENDENCIES = $(LIB_DIR)/libraft.a $(LIB_DIR)/libkvraft-rpc.a $(LIB_DIR)/libkvraft.a $(LIB_DIR)/libshardkvraft.a

all: make-shardkv $(BIN)/test_shardkv

$(BIN)/test_shardkv: test_shardkv.cpp $(DEPENDENCIES) $(INC_DIR)/tools/*.hpp
	$(CC) $< $(CPPFLAGS) $(LIBS) $(LDFLAGS) $(INC) -o $@ 

.PHONY: clean clean-log-port make-shardkv run-test
clean: clean-log-port
	rm -rf $(BIN)/test_shardkv $(LIB_DIR)/*raft*.a

clean-log-port:
	rm -rf $(ROOTDIR)/logs/*
	for port in 1 2 3 4 5 6 7; do \
		pid=`lsof -i:700$$port -t`;		  \
		if [ "$$pid" ]; \
		then \
			kill -s 9 $$pid; \
		fi \
	done 

make-shardkv:
	make -C $(SRC_DIR)/rpc
	make -C $(SRC_DIR)/raft
	make -C $(SRC_DIR)/kvraft
	make -C $(SRC_DIR)/shardkv

run-test: make-shardkv $(BIN)/test_shardkv
	$(BIN)/test_shardkv $(cmd_args)