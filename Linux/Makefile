megalan:
	g++ -O3 -o megalan *.cpp -ldl -lcrypto -lssl -std=c++11
install: megalan
	cp megalan /bin/megalan
debug: clean
	g++ -g -o megalan *.cpp -ldl -lcrypto -lssl -std=c++11
clean:
	rm -f megalan
