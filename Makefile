all:	site-tester

site-tester: site-tester.cpp
	g++ site-tester.cpp -std=c++11 -lcurl -o site-tester

clean:
	rm *.csv site-tester