default: server

server: hw2.cpp
	clang++ hw2.cpp libunp.a -o word_guess.out

clean:
	rm -f *.o *.out
