#include <unistd.h>
#include <fcntl.h>

int main()
{
	unsigned char buf[256];
	int fd;
	fd = open("data/bench_sample1.bin", O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);

	for(int i = 0; i < 256; i++)
	{
		buf[i] = i;
	}

	for(int i = 0; i < 33554432; i++)
	{
		write(fd, buf, sizeof(buf));
	}

	close(fd);
}
