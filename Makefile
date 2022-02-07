run: sendfile.o
	g++ sendfile.o -o run

sendfile.o: sendfile.cpp
	g++ sendfile.cpp -c

clean:
	rm *.o
