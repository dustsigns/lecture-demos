#Common base for all Makefiles
# Andreas Unterweger, 2016-2018
#This code is licensed under the 3-Clause BSD License. See LICENSE file for details.

#The directory that this file is contained in ($(CURDIR) does not change on include)
COMMONPATH := $(dir $(lastword $(MAKEFILE_LIST)))

include $(COMMONPATH)/tools.mak

DEBUG ?= 1

CXXFLAGS += -c -std=c++14
ifeq ($(DEBUG), 1)
  CXXFLAGS += -g -Wall -Wextra -Wpedantic -Werror
else
  CXXFLAGS += -O3 -flto -march=native -mtune=native
endif

LDFLAGS += -pthread
ifneq ($(DEBUG), 1)
  LDFLAGS += -O3 -flto
endif

CXXFLAGS += -I$(COMMONPATH)/comutils
SRCDEP := $(wildcard $(COMMONPATH)/comutils/*.cpp)
ifneq ($(filter sound,$(PARTS)),)
  SRCDEP += $(wildcard $(COMMONPATH)/sndutils/*.cpp)
  CXXFLAGS += -I$(COMMONPATH)/sndutils
  LIBS += ao
endif
ifneq ($(filter images,$(PARTS)),)
  SRCDEP += $(wildcard $(COMMONPATH)/imgutils/*.cpp)
  CXXFLAGS += -I$(COMMONPATH)/imgutils
  LIBS += opencv
endif
ifneq ($(filter 3d,$(PARTS)),)
  SRCDEP += $(wildcard $(COMMONPATH)/3dutils/*.cpp)
  CXXFLAGS += -I$(COMMONPATH)/3dutils
  LIBS += opencv eigen3
  #TODO: Remove eigen3 as soon as it is added as a dependency in OpenCV's pkg-config file
endif
OBJDEP := $(SRCDEP:.cpp=.o)

ifneq ($(LIBS),)
	CXXFLAGS += `$(PKGCFG) --cflags $(LIBS)`
endif

ifneq ($(LIBS),)
	LDFLAGS += `$(PKGCFG) --libs $(LIBS)`
endif
LDFLAGS += -Wl,-rpath,/usr/local/lib

SRC := $(wildcard *.cpp)
OBJ := $(SRC:.cpp=.o)

%.o: %.cpp
	$(CXX) $< -o $@ $(CXXFLAGS)

clean::
	$(RM) $(OBJ)
