#include "kernel/types.h"
#include "user/user.h"

int main() {
	int p1[2], p2[2];
	char buf[10];
	pipe(p1);
	pipe(p2);
	int child = fork();
	if (child != 0) { // 父亲
		write(p2[1], " ", 1);
		read(p1[0], buf, 1);
		fprintf(1, "%d: received pong\n", getpid());
	} else { // 儿子
		read(p2[0], buf, 1);
		fprintf(1, "%d: received ping\n", getpid());
		write(p1[1], buf, 1);
	}
	exit(0);
}