-include ../../Global.mk

ROOTDIR = ../..

all: $(LIBDIR)/libkvraft-rpc.a $(LIBDIR)/libmr-rpc.a

$(LIBDIR)/libkvraft-rpc.a: KVRaft
	$(AR) $@ $</*.o

$(LIBDIR)/libmr-rpc.a: MapReduce
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
	rm -rf $(LIBDIR)/libkvraft-rpc.a KVRaft
