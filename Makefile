CXX=gcc
CXXOPTIMIZE= -O2
CXXFLAGS= -g -Wall -pthread -std=c11 $(CXXOPTIMIZE)
USERID=EDIT_MAKE_FILE
CLASSES=

all: web-server web-client

clean:
	rm -rf *.o *~ *.gch *.swp *.dSYM web-server web-client *.tar.gz

tarball: clean
	tar -cvf $(USERID).tar.gz *
