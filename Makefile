# Requires the OpenNCC Frame native SDK installed via its install.sh
# (headers in /usr/include/openncc, library in /usr/lib/openncc).
CXX      ?= g++
CXXFLAGS ?= -std=c++11 -O2 -Wall -Wextra
NCC_INC   = /usr/include/openncc
NCC_LIB   = /usr/lib/openncc
LDLIBS    = $(NCC_LIB)/libOpenNCC_native.so -lusb-1.0 -lpthread
LDFLAGS   = -Wl,-rpath,$(NCC_LIB)

ncc-keepalive: ncc-keepalive.cpp
	$(CXX) $(CXXFLAGS) -I$(NCC_INC) -o $@ $< $(LDFLAGS) $(LDLIBS)

clean:
	rm -f ncc-keepalive

.PHONY: clean
