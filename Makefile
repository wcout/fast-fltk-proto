FLTK := ../fltk-1.3
FLTK_CONFIG := $(FLTK)/fltk-config

SRC :=  fast_proto.cxx
OBJ := $(SRC:.cxx=.o)

all:
	$(FLTK_CONFIG) --use-images --compile $(SRC)
