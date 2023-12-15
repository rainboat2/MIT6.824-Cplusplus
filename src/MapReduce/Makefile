-include ../../Global.mk

ROOTDIR = ../..

LIBS   := -lmr-rpc -lkvraft-rpc -ldl $(LIBS) 

all: $(BIN)/master $(BIN)/worker $(BIN)/WordCount.so

$(BIN)/%: %.cpp
	$(CC) $< $(CPPFLAGS) $(LIBS) $(LDFLAGS) $(INC) -o $@

$(BIN)/%.so : user-program/%.cpp
	$(CC) $(CPPFLAGS) $(SHAREDFLAGS) $(INC) -o $@ $<

.PHONY: clean all
clean:
	rm -rf $(BIN)/master $(BIN)/worker user-program/*.so
