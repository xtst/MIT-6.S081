#include "kernel/fs.h"
#include "kernel/param.h"
#include "kernel/stat.h"
#include "kernel/types.h"
#include "user/user.h"

int main(int argc, char *argv[]) {
	char buf[1005] = {0};
	char *p = buf, *pbuf[1005], *last_end = buf;
	char **args = pbuf;
	for (int i = 1; i < argc; i++) {
		*args = argv[i];
		args++;
	}
	char **p_array = args;
	while (read(0, p, 1) == 1) {
		// printf("===%c====\n", *p);
		if (*p == '\n' || *p == ' ') {
			*p = '\0';
			*(p_array++) = last_end;
			last_end = p + 1;

			if (*p == ' ') continue;
			if (fork() == 0) {
				*p_array = 0;
				exec(argv[1], pbuf);
				exit(0);
			} else
				p_array = args;
		}
		p++;
	}
	if (p_array != args) {
		*p = '\0';
		*(p_array++) = last_end;
		*p_array = 0;
		if (fork() == 0) {
			exec(argv[1], pbuf);
			exit(0);
		}
	}
	while (wait(0) != -1) {}
	exit(0);
}