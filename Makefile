TARGETS = MapReduce raft kvraft
.PHONY: $(TARGETS)

all: $(TARGETS)

MapReduce:
	make -C src/$@

raft: rpc
	make -C src/$@

kvraft: rpc
	make -C src/$@

rpc:
	make -C src/$@

clean:
	rm -rf objs/*
	make -C src/MapReduce clean
	make -C src/raft clean 
	make -C src/kvraft clean
	make -C src/rpc clean
