planesmake: planes.c
	gcc -o planes planes.c -lpthread
	./planes