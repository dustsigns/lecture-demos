#Makefile for all applications (subfolders)
# Andreas Unterweger, 2016-2018
#This code is licensed under the 3-Clause BSD License. See LICENSE file for details.

include common/tools.mak

ALLDIR := $(wildcard */)
ALLDIR := $(ALLDIR:%/=%)

DATADIR := screenshots testdata
CODEDIR := $(filter-out $(DATADIR), $(ALLDIR))
COMMONDIR := common
DIR := $(filter-out $(COMMONDIR), $(CODEDIR))

BUILDDIRCMD := $(addprefix build-, $(DIR))
CLEANDIRCMD := $(addprefix clean-, $(CODEDIR))
TSTDIRCMD := $(addprefix test-, $(DIR))

build-%:
	$(CD) $* && $(MAKE)
	
clean-%:
	$(CD) $* && $(MAKE) clean

.DEFAULT_GOAL := all

all: $(BUILDDIRCMD)

clean: $(CLEANDIRCMD)

test-%:
	$(CD) $* && $(MAKE) tests

tests: $(TSTDIRCMD)
