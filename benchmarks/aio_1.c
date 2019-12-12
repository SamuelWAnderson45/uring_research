#define _GNU_SOURCE
#include <libaio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <assert.h>
#include <sys/uio.h>
#include <sys/stat.h>
#include <limits.h>

#define QUEUE_NUM 8
#define BUF_SIZE 4096
#define IOVEC_NUM 8
#define BLOCK_SIZE (BUF_SIZE*QUEUE_NUM*IOVEC_NUM)

static unsigned char buffers[QUEUE_NUM*IOVEC_NUM][BUF_SIZE] __attribute__ ((aligned (512)));
static struct iocb io_ops[QUEUE_NUM];
static struct iocb* io_ops_ptrs[QUEUE_NUM];
static struct io_event events[QUEUE_NUM];
static struct iovec iov[IOVEC_NUM*QUEUE_NUM];

int main(int argc, char** argv)
{
	io_context_t ctx;
	int fd, ret;
	struct stat sample_stat;

	fd = open("data/bench_sample1.bin", O_RDONLY);
	if(fd < 0)
	{
		perror("open");
		return 1;
	}
	fstat(fd, &sample_stat);
	assert(sample_stat.st_size % BUF_SIZE*QUEUE_NUM*IOVEC_NUM == 0);

	memset(&ctx, 0, sizeof(ctx));
	ret = io_setup(QUEUE_NUM, &ctx);
	if(ret < 0)
	{
		fprintf(stderr, "io_setup failed with %d\n", ret);
		return 1;
	}

	// must set to O_DIRECT, otherwise will block in io_submit
	ret = fcntl(fd, F_GETFL);
	if(ret == -1)
	{
		perror("fcntl");
		return 1;
	}
	ret = fcntl(fd, F_SETFL, ret | O_DIRECT);
	if(ret == -1)
	{
		perror("fcntl");
		return 1;
	}

	for(int i = 0; i < QUEUE_NUM; i++)
	{
		io_ops_ptrs[i] = &io_ops[i];
	}

	long long blockNum = sample_stat.st_size / BLOCK_SIZE;
	for(long long block = 0; block < blockNum; block++)
	{
		for(long long queue = 0; queue < QUEUE_NUM; queue++)
		{
			for(long long vec = 0; vec < IOVEC_NUM; vec++)
			{
				struct iovec tmp;
				tmp.iov_base = buffers[queue*IOVEC_NUM + vec];
				tmp.iov_len = BUF_SIZE;
				iov[queue*IOVEC_NUM + vec] = tmp;
			}
			io_prep_preadv(&io_ops[queue], fd, &iov[queue*IOVEC_NUM], IOVEC_NUM, block*BLOCK_SIZE + queue*IOVEC_NUM*BUF_SIZE);
		}
		ret = io_submit(ctx, QUEUE_NUM, io_ops_ptrs);
		if(ret != QUEUE_NUM)
		{
			printf("block %lld %d\n", block, ret);
			perror("io_submit");
			return 1;
		}
		//do work here

		//for simplicity, we will block until all queued events are finished.
		ret = io_getevents(ctx, QUEUE_NUM, QUEUE_NUM, events, NULL);
		if(ret != QUEUE_NUM)
		{
			printf("Missed events\n");
			return 1;
		}
		for(int i = 0; i < QUEUE_NUM; i++)
		{
			if(events[i].res < 0)
			{
				printf("event failed: block %lld event %d\n", block, i);
				return 1;
			}
		}
	}

	close(fd);
}
