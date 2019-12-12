all: gen io_uring aio

gen:
	gcc -o gen gen.c

io_uring: io_uring_1.c
	gcc -o io_uring io_uring_1.c -luring

aio: aio_1.c
	gcc -o aio aio_1.c -laio
