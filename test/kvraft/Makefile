include ../../Global.mk

ROOTDIR = ../..
LIBS   := $(LIBS) -lkvraft-rpc -lgtest -lgtest_main

KVRAFT_SOURCES = test_kvraft.cpp

all: $(BIN)/test_kvraft

$(BIN)/test_kvraft: $(KVRAFT_SOURCES)
	$(CC) $(CPPFLAGS) $(LIBS) $(LDFLAGS) $(INC) -o $@ $<

.PHONY: clean clean-log-port make-kvraft run-test
clean:
	rm -rf $(BIN)/kvraft_test

clean-log-port:
	rm -rf $(ROOTDIR)/logs/*
	for port in 1 2 3 4 5 6 7; do \
		lsof -i:700$$i -t | xargs kill -9; \
	done 

make-kvraft:
	make -C $(SRC_DIR)/rpc
	make -C $(SRC_DIR)/raft
	make -C $(SRC_DIR)/kvraft

run-test: $(BIN)/test_kvraft make-kvraft
	$(BIN)/test_kvraft $(cmd_args)