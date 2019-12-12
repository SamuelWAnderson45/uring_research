#!/bin/bash

echo Warming disk cache
time ./io_uring_1

echo Testing io_uring \(buffered\)
for i in {1..10}
do
	time ./io_uring_1
done

echo Testing aio \(direct\)
for i in {1..10}
do
	time ./aio_1
done

