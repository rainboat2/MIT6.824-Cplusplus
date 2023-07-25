-include ../../Global.mk

ROOTDIR = ../..

all: $(LIB_DIR)/libkvraft-rpc.a $(LIB_DIR)/libmr-rpc.a

$(LIB_DIR)/libkvraft-rpc.a: KVRaft
	$(AR) $@ $</*.o

$(LIB_DIR)/libmr-rpc.a: MapReduce
	$(AR) $@ $</*.o

KVRaft: KVRaft.thrift
	rm -rf $@
	thrift -r --gen cpp $<
	mv gen-cpp $@
	$(CC) $(CPPFLAGS) $(INC) -c $@/KVRaft_types.cpp -o $@/KVRaft_types.o
	$(CC) $(CPPFLAGS) $(INC) -c $@/Raft.cpp -o $@/Raft.o
	$(CC) $(CPPFLAGS) $(INC) -c $@/KVRaft.cpp -o $@/KVRaft.o
	cp $@/*.h $(INC_DIR)/rpc/kvraft

MapReduce: MapReduce.thrift
	rm -rf $@
	thrift -r --gen cpp $<
	mv gen-cpp $@
	$(CC) $(CPPFLAGS) $(INC) -c $@/MapReduce_types.cpp -o $@/MapReduce_types.o
	$(CC) $(CPPFLAGS) $(INC) -c $@/Master.cpp -o $@/Master.o
	cp $@/*.h $(INC_DIR)/rpc/mapreduce

clean:
	rm -rf $(LIB_DIR)/libkvraft-rpc.a KVRaft
