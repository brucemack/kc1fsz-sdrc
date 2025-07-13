INC=-Isrc -Iradlib -Ikc1fsz-tools-cpp/include

main:	test-AudioCore.o AudioCore.o dsp_util.o
	g++ -o main test-AudioCore.o AudioCore.o dsp_util.o

test-AudioCore.o: src/test/test-AudioCore.cpp
	g++ -std=c++20 $(INC) -c src/test/test-AudioCore.cpp

AudioCore.o:	src/AudioCore.cpp src/AudioCore.h
	g++ -std=c++20 $(INC) -c src/AudioCore.cpp

dsp_util.o:	radlib/util/dsp_util.h radlib/util/dsp_util.cpp
	g++ -std=c++20 $(INC) -c radlib/util/dsp_util.cpp

clean:
	rm -f test-AudioCore.o AudioCore.o
