CXX ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra -Wpedantic
LDFLAGS ?= -lX11

all: shitwave

shitwave: src/main.cpp
	$(CXX) $(CXXFLAGS) -o $@ $< $(LDFLAGS)

run: shitwave
	./shitwave

clean:
	rm -f shitwave

.PHONY: all run clean
