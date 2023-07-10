ROOTDIR     = ../..
-include ../../Global.mk

LIBS := $(LIBS) -lgtest -lgtest_main -lraft

RAFT_LIB = $(LIBDIR)/libraft.a

all: make-raft $(BIN)/test_raft $(BIN)/test

$(BIN)/%: %.cpp $(RAFT_LIB) RaftProcess.hpp
	$(CC) $(CPPFLAGS) $(LIBS) $(LDFLAGS) $(INC) -o $@ $<

$(RAFT_LIB):
	make -C $(ROOTDIR)/raft

.PHONY: clean run-test make-raft
clean:
	rm $(BIN)/test_raft

run-test: $(BIN)/test_raft
	$(BIN)/test_raft

make-raft:
	make -C $(ROOTDIR)/raft
