CC	= gcc
CFLAGS	= -O3
BIN_DIR	= ./bin
SRC_DIR	= ./src
DOC_DIR	= ./docs
INC_DIR	= ./include
LIB_DIR	= ./lib
BIN	= siftfeat match dspfeat db-add db-list db-match db-del

all: $(BIN) libsift.a

docs:
	doxygen Doxyfile

libsift.a:
	make -C $(SRC_DIR) $@

$(BIN):
	make -C $(SRC_DIR) $@

clean:
	make -C $(SRC_DIR) $@;	\
	make -C $(INC_DIR) $@;	\

distclean: clean
	rm -f $(LIB_DIR)/*
	rm -rf $(DOC_DIR)/html/
	rm -f $(BIN_DIR)/*

.PHONY: docs clean libsift.a
