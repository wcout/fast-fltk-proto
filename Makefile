# If using a not installed FLTK version, specify it's path here (with '/' at end)
#FLTK := ../fltk-1.3/
FLTK_CONFIG := $(FLTK)fltk-config

SRC :=  fast_proto.cxx
OBJ := $(SRC:.cxx=.o)

all:
	$(FLTK_CONFIG) --use-images --compile $(SRC)
