all:
	gcc -g -c -o obj/customer.o src/customer.c
	gcc -g -c -o obj/teller.o src/teller.c
	gcc -g -c -o obj/event.o src/event.c
	gcc -g -c -o obj/qSim.o src/qSim.c
	gcc -g -o qSim obj/qSim.o obj/customer.o obj/teller.o obj/event.o -lm
clean:
	rm qSim
	rm obj/*
