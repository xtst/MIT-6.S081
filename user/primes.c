#include "kernel/types.h"
#include "user/user.h"

void transmit_data(int p_left[]) {
	int p_right[2];
	int n, return_val = 1, start;
	return_val = read(p_left[0], &start, 4);
	if (return_val == 0) { exit(0); }
	printf("prime %d\n", start);

	pipe(p_right);

	int pid = fork();
	if (pid == 0) {
		close(p_right[1]);
		close(p_left[0]);
		transmit_data(p_right);
		exit(0);
	} else {
		close(p_right[0]);
		while (1) {
			return_val = read(p_left[0], &n, 4);
			if (return_val == 0) { break; }
			if (n % start != 0) write(p_right[1], &n, 4);
		}
		close(p_right[1]);
		wait(0);
		exit(0);
	}
	exit(0);
}

int main() {
	int p_left[2];
	pipe(p_left);
	if (fork() == 0) {
		close(p_left[1]);
		transmit_data(p_left);
	} else {
		close(p_left[0]);
		for (int i = 2; i <= 31; i++) {
			write(p_left[1], &i, 4);
		}
		close(p_left[1]);
	}
	wait(0);
	exit(0);
}