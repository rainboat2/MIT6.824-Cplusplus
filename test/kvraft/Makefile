include ../../Global.mk

ROOTDIR = ../..
LIBS   := $(LIBS) -lkvraft-rpc -lkvraft -lraft -lgtest -lgtest_main -lpthread

DEPENDENCIES = $(LIB_DIR)/libraft.a $(LIB_DIR)/libkvraft-rpc.a $(LIB_DIR)/libkvraft.a KVRaftProcess.hpp

all: make-kvraft $(BIN)/test_kvraft

$(BIN)/test_kvraft: test_kvraft.cpp $(DEPENDENCIES)
	$(CC) $< $(CPPFLAGS) $(LIBS) $(LDFLAGS) $(INC) -o $@ 

.PHONY: clean clean-log-port make-kvraft run-test
clean: clean-log-port
	rm -rf $(BIN)/test_kvraft $(LIB_DIR)/*raft*.a

clean-log-port:
	rm -rf $(ROOTDIR)/logs/*
	for port in 1 2 3 4 5 6 7; do \
		pid=`lsof -i:700$$port -t`;		  \
		if [ "$$pid" ]; \
		then \
			kill -s 9 $$pid; \
		fi \
	done 


make-kvraft:
	make -C $(SRC_DIR)/rpc
	make -C $(SRC_DIR)/raft
	make -C $(SRC_DIR)/kvraft

run-test: make-kvraft $(BIN)/test_kvraft
	$(BIN)/test_kvraft $(cmd_args)