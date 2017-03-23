all:	site-tester

site-tester: site-tester.o
	g++ site-tester.o -lm -lcurl -o site-tester

site-tester.o: site-tester.cpp
	g++ -c site-tester.cpp

clean:
	rm *.o site-tester