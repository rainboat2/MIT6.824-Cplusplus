TARGETS = MapReduce raft kvraft rpc shardkv
.PHONY: $(TARGETS) count-line clean

all: $(TARGETS)

MapReduce:
	make -C src/$@

raft: rpc
	make -C src/$@

kvraft: rpc
	make -C src/$@

rpc:
	make -C src/$@

shardkv:
	make -C src/$@

count-line:
	find . -type f    \
		| grep -E ".*\.(cpp|h|hpp|sh|py|thrift)|Makefile"   \
		| grep -v -E "rpc/(MapReduce|KVRaft)/.*"  \
		| grep -v "include/rpc"  \
		| xargs wc -l \

clean:
	rm -rf objs/*
	make -C src/MapReduce clean
	make -C src/raft clean 
	make -C src/kvraft clean
	make -C src/shardkv clean
	make -C src/rpc clean
