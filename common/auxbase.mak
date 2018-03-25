#Base Makefile for all common folders (source code only)
# Andreas Unterweger, 2018
#This code is licensed under the 3-Clause BSD License. See LICENSE file for details.

#The directory that this file is contained in ($(CURDIR) does not change on include)
CURRENTPATH := $(dir $(lastword $(MAKEFILE_LIST)))

include $(CURRENTPATH)common.mak

.DEFAULT_GOAL := all

all: $(OBJ)
