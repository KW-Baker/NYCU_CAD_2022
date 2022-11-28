all: 310510221.cpp
	g++ -std=c++11 310510221.cpp -o 310510221.o
clean:
	rm -f 310510221.o
run:
	./310510221.o example.v -p example.pat -l test_lib.lib
	./310510221.o c17.v -p c17.pat -l test_lib.lib
	./310510221.o c432.v -p c432.pat -l test_lib.lib

run_example:
	./310510221.o example.v -p example.pat -l test_lib.lib

run_c17:
	./310510221.o c17.v -p c17.pat -l test_lib.lib

run_c432:
	./310510221.o c432.v -p c432.pat -l test_lib.lib