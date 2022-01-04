#Base Makefile for all folders with applications (source code plus executables)
# Andreas Unterweger, 2018-2022
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

$(TST): test_%: %.exe %_test
	@while read test; \
	do \
		$(ECHO) ./$< $$test; \
		./$< $$test; \
	done < $*_test

.PHONY: $(TST) tests ordered_tests all clean

tests: $(TST)

ORDERED_TESTS := $(addprefix test_, $(ORDER))

COUNT_ORDERED_TESTS := $(words $(ORDERED_TESTS))
COUNT_TESTS := $(words $(TST))
ifneq ($(COUNT_ORDERED_TESTS), $(COUNT_TESTS))
  $(warning Demonstrations may be missing when using the ordered_tests target.)
  $(warning $(COUNT_ORDERED_TESTS) ordered vs. $(COUNT_TESTS) available demonstrations.)
endif

ordered_tests: $(ORDERED_TESTS)

.DEFAULT_GOAL := all

all: $(EXE)

clean::
	$(RM) $(EXE)
