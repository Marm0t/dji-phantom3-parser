all: parser

parser: parser.o packet.o
	    g++ parser.o packet.o -o parser

parser.o: parser.cpp
	    g++ -c parser.cpp

packet.o: packet.cpp
	    g++ -c packet.cpp

clean:
	    rm *o parser
