-include ../../Global.mk
ROOTDIR     = ../..

LIBS := $(LIBS) -lgtest -lgtest_main -lraft -lkvraft-rpc

RAFT_LIB = $(LIB_DIR)/libraft.a

all: $(BIN)/test_raft

$(BIN)/%: %.cpp $(RAFT_LIB) RaftProcess.hpp
	$(CC) $(CPPFLAGS) $(LIBS) $(LDFLAGS) $(INC) -o $@ $<

$(RAFT_LIB):
	make -C $(SRC_DIR)/raft

.PHONY: clean run-test make-raft clean-log-port
clean: clean-log-port
	rm $(BIN)/test_raft
	make -C $(SRC_DIR)/raft clean

run-test: make-raft $(BIN)/test_raft
	$(BIN)/test_raft $(cmd_args)

make-raft:
	make -C $(SRC_DIR)/raft

clean-log-port:
	rm -rf $(ROOTDIR)/logs/*
	for port in 1 2 3 4 5 6 7; do \
		lsof -i:700$$port -t | xargs kill -9; \
	done 
