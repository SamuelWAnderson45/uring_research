#define _GNU_SOURCE
#include <liburing.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <assert.h>
#include <sys/uio.h>
#include <sys/stat.h>
#include <limits.h>
#include <string.h>

#define QUEUE_NUM 8
#define BUF_SIZE 4096
#define IOVEC_NUM 8
#define SAM_BLOCK_SIZE (BUF_SIZE*QUEUE_NUM*IOVEC_NUM)

static unsigned char buffers[QUEUE_NUM*IOVEC_NUM][BUF_SIZE] __attribute__ ((aligned (512)));
static struct io_uring_cqe* events[QUEUE_NUM];
static struct iovec iov[IOVEC_NUM*QUEUE_NUM];

int main(int argc, char** argv)
{
	struct io_uring ring;
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

	ret = io_uring_queue_init(QUEUE_NUM, &ring, 0);
	if(ret < 0)
	{
		fprintf(stderr, "queue_init failed with %d\n", ret);
		return 1;
	}

	long long blockNum = sample_stat.st_size / SAM_BLOCK_SIZE;
	for(long long block = 0; block < blockNum; block++)
	{
		for(long long queue = 0; queue < QUEUE_NUM; queue++)
		{
			struct io_uring_sqe* sqe;
			for(long long vec = 0; vec < IOVEC_NUM; vec++)
			{
				struct iovec tmp;
				tmp.iov_base = buffers[queue*IOVEC_NUM + vec];
				tmp.iov_len = BUF_SIZE;
				iov[queue*IOVEC_NUM + vec] = tmp;
			}
			sqe = io_uring_get_sqe(&ring);
			io_uring_prep_readv(sqe, fd, &iov[queue*IOVEC_NUM], IOVEC_NUM, block*SAM_BLOCK_SIZE + queue*IOVEC_NUM*BUF_SIZE);
		}
		ret = io_uring_submit_and_wait(&ring, QUEUE_NUM);
		struct io_uring_cqe *cqe;
		unsigned int head;
		int i = 0;
		io_uring_for_each_cqe(&ring, head, cqe) {
			if (cqe->res < 0) {
				printf("event failed: block %lld event %d\n", block, i);
				return 1;
			}
			i++;
		}
		io_uring_cq_advance(&ring, QUEUE_NUM);
	}

	close(fd);
	io_uring_queue_exit(&ring);
}
