# specify path to FLTK 1.3-x (mingw compiled)
FLTK_DIR=../../fltk-1.3-mingw32/
USE_FLTK_RUN=0
HAVE_SLOW_CPU=0

export ROOT?=$(PWD)/..

HOST=i686-w64-mingw32
CXX=$(shell sh -c 'which $(HOST)-g++')

ifeq ($(CXX),)
HOST=i586-pc-mingw32
CXX=$(shell sh -c 'which $(HOST)-g++')
endif

ifeq ($(CXX),)
HOST=i586-mingw32msvc
CXX=$(shell sh -c 'which $(HOST)-g++')
endif

ifeq ($(CXX),)
all clean install res:
	@tput setaf 1
	@echo "cross compiler for $(HOST) not installed."
	@tput sgr0
else
WINDRES=$(shell sh -c 'which $(HOST)-windres')

APPLICATION=fast_proto

FLTK_CONFIG=$(FLTK_DIR)fltk-config

# change path as required...
INSTALL_PATH=$(HOME)/Downloads/$(APPLICATION)
RSC_PATH=$(INSTALL_PATH)
TARGET_ROOT=$(HOME)

SRC=$(ROOT)

OBJ1=\
	$(APPLICATION).o

INCLUDE=-I$(ROOT)/include -I.

LDFLAGS=
LDLIBS=`$(FLTK_CONFIG) --use-images --ldstaticflags` -static -lgcc -static -lstdc++ -static -lwinmm
LDLIBSCPP=-static -lgcc -static -lstdc++ -dynamic -static -lwinmm

FLTKCXXFLAGS = `$(FLTK_CONFIG) --cxxflags`
CXXDEFS+=-DUSE_FLTK_RUN=$(USE_FLTK_RUN)
CXXDEFS+=-DHAVE_SLOW_CPU=$(HAVE_SLOW_CPU)
CXXFLAGS+=$(CXXDEFS) -g -Wall -pedantic $(INCLUDE) $(FLTKCXXFLAGS)
#OPT=
OPT=-O3 -DNDEBUG

CP=cp
MV=mv
RM=rm
MKDIR=mkdir
PATCH=patch
TAR=tar

TARGET1=$(APPLICATION).exe

export TARGET_NAME=$(APPLICATION)-win32
export TARGET_PATH=$(TARGET_ROOT)/$(TARGET_NAME)

.PHONY: clean all target

all:: $(TARGET1) $(TARGET2) $(TARGET3)

$(TARGET1): depend $(OBJ1)
	@echo Linking $@...
	$(CXX) -o $@ $(LDFLAGS) $(OBJ1) $(LDLIBS)

%.o: $(SRC)/%.cxx
	@echo Compiling $@...
	$(CXX) -c -o $@ $< $(OPT) $(CXXFLAGS)

%.res: %.rc
	@echo Compiling $@...
	@echo Make resource $@..
	$(WINDRES) $< -O coff -o $@

clean::
	$(RM) -f $(ALL) *.o *.res core makedepend depend *~
	$(RM) -f $(TARGET1)

res: *.rc
	@echo Make resources..
	$(WINDRES) $(APPLICATION).rc -O coff -o $(APPLICATION).res

depend: $(SRC)/*.cxx
	@echo Make dependencies..
	$(CXX) $(CXXFLAGS) -M $(SRC)/*.cxx >makedepend
	# only a dummy
	touch depend

-include makedepend

endif
