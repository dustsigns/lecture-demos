#Common base for all Makefiles
# Andreas Unterweger, 2016-2023
#This code is licensed under the 3-Clause BSD License. See LICENSE file for details.

#The directory that this file is contained in ($(CURDIR) does not change on include)
COMMONPATH := $(dir $(lastword $(MAKEFILE_LIST)))

include $(COMMONPATH)/tools.mak

DEBUG ?= 1

CXXFLAGS += -c -std=c++17
ifeq ($(DEBUG), 1)
  CXXFLAGS += -g -Wall -Wextra -Werror
  #CXXFLAGS += -Wpedantic #Uncomment once OpenCV's stitching header does not cause warnings/errors anymore
else
  CXXFLAGS += -O3 -flto -march=native -mtune=native
endif

LDFLAGS += -pthread
ifneq ($(DEBUG), 1)
  LDFLAGS += -O3 -flto
endif

SRCDEPS := comutils imgutils
LIBS += opencv4
ifneq ($(filter sound,$(PARTS)),)
  SRCDEPS += sndutils
  LIBS += ao
endif
ifneq ($(filter 3d,$(PARTS)),)
  SRCDEPS += 3dutils
endif
SRCDEPS := $(addprefix $(COMMONPATH)/, $(SRCDEPS))
CXXFLAGS += $(addprefix -I, $(SRCDEPS))
SRCDEP := $(wildcard $(addsuffix /*.cpp, $(SRCDEPS)))
OBJDEP := $(SRCDEP:.cpp=.o)

ifneq ($(LIBS),)
	CXXFLAGS += `$(PKGCFG) --cflags $(LIBS)`
endif

ifneq ($(LIBS),)
	LDFLAGS += `$(PKGCFG) --libs $(LIBS)`
endif

SRC := $(wildcard *.cpp)
OBJ := $(SRC:.cpp=.o)

%.o: %.cpp
	$(CXX) $< -o $@ $(CXXFLAGS)

clean::
	$(RM) $(OBJ)
