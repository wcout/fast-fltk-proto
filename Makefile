# If using a not installed FLTK version, specify it's path here
#FLTK := ../fltk-1.4

FLTK_CONFIG := fltk-config
ifdef FLTK
FLTK_CONFIG := $(realpath $(FLTK))/$(FLTK_CONFIG)
endif

SRC :=  fast_proto.cxx
OBJ := $(SRC:.cxx=.o)
TGT := $(SRC:.cxx=)

all:
	$(CXX) -g -O2 -DFLTK_CONFIG=$(FLTK_CONFIG) -Wall -o $(TGT) `$(FLTK_CONFIG) --use-images --cxxflags` $(SRC) `$(FLTK_CONFIG) --use-images --ldflags`
