#include "kernel/fs.h"
#include "kernel/stat.h"
#include "kernel/types.h"
#include "user/user.h"

char *fmtname(char *path) {
	// printf("----%s------\n", path);
	char *buf;
	buf = malloc(60);
	memset(buf, 0, sizeof(buf));
	int len = 0;
	for (int i = strlen(path) - 1; i >= 0; i--) {
		if (path[i] != '/')
			buf[len++] = path[i];
		else
			break;
	}
	for (int i = 0; i < len / 2; i++) {
		char temp;
		temp = buf[i];
		buf[i] = buf[len - i - 1];
		buf[len - i - 1] = temp;
	}
	buf[len] = '\0';
	return buf;
}

void find(char *path, char *target) {
	// printf("======asdfasdf %s %s\n", path, target);
	char buf[512] = {0}, *p;
	int fd;
	// int  queue[100], r = 0;
	struct dirent de;
	struct stat st;

	if ((fd = open(path, 0)) < 0) {
		fprintf(2, "ls: cannot open %s\n", path);
		return;
	}

	// printf("fucker\n");
	if (fstat(fd, &st) < 0) {
		fprintf(2, "ls: cannot stat %s\n", path);
		close(fd);
		return;
	}

	// char stemp[512];
	switch (st.type) {
	case T_FILE:
		// printf("%s %d %d %l\n", fmtname(path), st.type, st.ino, st.size);
		// int flag = 0;
		// strcpy(stemp, fmtname(path));
		// // printf("==== %s  %s\n", fmtname(path), stemp);
		// for (int i = strlen(stemp) - 1; i >= 0; i--) {
		// 	if (stemp[i] == '.') {
		// 		stemp[i] = '\0';
		// 		break;
		// 	}
		// }
		// printf("----PATH: _%s_  FILE: _%s_  TARGET: _%s_\n", path, stemp, target);
		// if (strcmp(stemp, target) == 0) {
		// 	printf("%s\n", buf);
		// }
		if (strcmp(path + strlen(path) - strlen(target), target) == 0) {
			printf("%s\n", path);
		}
		break;

	case T_DIR:
		// printf("1212121212\n");
		strcpy(buf, path);
		p = buf + strlen(buf);
		*p++ = '/';
		while (read(fd, &de, sizeof(de)) == sizeof(de)) {
			// printf("----PATH: _%s_  DIR: _%s_  TARGET: _%s_\n", path, fmtname(path), target);
			if (de.inum == 0)
				continue;
			memmove(p, de.name, DIRSIZ);
			p[DIRSIZ] = 0;
			if (stat(buf, &st) < 0) {
				printf("ls: cannot stat %s\n", buf);
				continue;
			}
			// printf("%s %d %d %d\n", fmtname(buf), st.type, st.ino, st.size);
			// int flag = 0;
			// printf("no : __%s__  %s %d\n", fmtname(buf), buf, strcmp(fmtname(buf), "."));

			// printf("ee\n");
			// if (strcmp(fmtname(buf), target) == 0) {
			// 	printf("%s\n", buf);
			// }
			// // printf("be\n");
			// if (strcmp(fmtname(buf), ".") != 0 && strcmp(fmtname(buf), "..") != 0) {

			// 	// printf("```` %s %s\n", buf, target);
			// 	find(buf, target);
			// }
			if (strcmp(buf + strlen(buf) - 2, "/.") != 0 && strcmp(buf + strlen(buf) - 3, "/..") != 0) {
				find(buf, target);
			}

			// printf("su\n");
		}
		break;
	}
	close(fd);
}
int main(int argc, char *argv[]) {
	if (argc < 2) {
		// ls(".");
		printf("Error\n");
		exit(0);
	}
	find(argv[1], argv[2]);
	exit(0);
}
