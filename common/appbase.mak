#Base Makefile for all folders with applications (source code plus executables)
# Andreas Unterweger, 2018-2019
#This code is licensed under the 3-Clause BSD License. See LICENSE file for details.

#The directory that this file is contained in ($(CURDIR) does not change on include)
CURRENTPATH := $(dir $(lastword $(MAKEFILE_LIST)))

include $(CURRENTPATH)common.mak

#Keep intermediate object files to allow for parallel builds (make removes them otherwise)
.PRECIOUS: %.o

EXE := $(OBJ:.o=.exe)
TST := $(addprefix test_, $(EXE:.exe=))

%.exe: %.o $(OBJDEP)
	$(CXX) $^ -o $@ $(LDFLAGS)

#TODO: Make test_ targets show up for auto-complete (.PHONY will not work as it does not process the prerequisites)
test_%: %.exe %_test
	./$< $(shell $(CAT) $*_test)

tests: $(TST)

.DEFAULT_GOAL := all

all: $(EXE)

clean::
	$(RM) $(EXE)
