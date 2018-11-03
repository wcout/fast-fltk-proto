# If using a not installed FLTK version, specify it's path here
#FLTK := ../fltk-1.4

# use FLTK line highlight patch (STR-3453)
#OPT=-DHAVE_LINE_HIGHLIGHT

FLTK_CONFIG := fltk-config
ifdef FLTK
FLTK_CONFIG := $(realpath $(FLTK))/$(FLTK_CONFIG)
endif

SRC :=  fast_proto.cxx
OBJ := $(SRC:.cxx=.o)
TGT := $(SRC:.cxx=)

all:
	$(CXX) -g -O2 -DFLTK_CONFIG=$(FLTK_CONFIG) $(OPT) -Wall -o $(TGT) `$(FLTK_CONFIG) --use-images --cxxflags` $(SRC) `$(FLTK_CONFIG) --use-images --ldflags`

install:
	cp -a $(TGT) ~/bin/.
