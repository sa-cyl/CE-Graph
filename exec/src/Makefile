INCFLAGS = -I/usr/local/include/ -I/cegraph/exec/src/

CPP = g++
CPPFLAGS = -g -O3 $(INCFLAGS)  -fopenmp -Wno-strict-aliasing
LINKERFLAGS = -lz
DEBUGFLAGS = -g -ggdb $(INCFLAGS)
HEADERS=$(wildcard *.h**)

ceMaster: ceMaster.cpp $(HEADERS)
	$(CPP) $(CPPFLAGS) ceMaster.cpp -o ceMaster $(LINKERFLAGS)
	mv ceMaster /cegraph/exec/bin

clean:
	@rm -rf ceMaster
