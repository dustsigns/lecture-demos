#Special Makefile for commonly used code (only used for cleaning and building on demand)
# Andreas Unterweger, 2016-2018
#This code is licensed under the 3-Clause BSD License. See LICENSE file for details.

include tools.mak

ALLDIR := $(wildcard */)
ALLDIR := $(ALLDIR:%/=%)

CLEANDIRCMD := $(addprefix clean-, $(ALLDIR))

.DEFAULT_GOAL := all

all:
	@echo "Common files are built implicitly on demand. They can only be cleaned with this Makefile."

clean-%:
	$(CD) $* && $(MAKE) clean

clean: $(CLEANDIRCMD)
