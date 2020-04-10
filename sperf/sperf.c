#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
/*
int main(int argc, char* argv[]){
 	char *exec_argv[] = { "strace", "ls", NULL, };
 	char *exec_envp[] = { "PATH=/bin", NULL, };
 	execve("strace",          exec_argv, exec_envp);
 	execve("/bin/strace",     exec_argv, exec_envp);
 	execve("/usr/bin/strace", exec_argv, exec_envp);
 	perror(argv[0]);
 	exit(EXIT_FAILURE);
}
*/
void child_proc(int fd, int argc, char* argv[], char* envp[]);
void parent_proc(int fd);

int main(int argc, char* argv[], char* envp[]) {
 	int pid_cp = -1;
 	int pipefd[2] = {};
 	pid_cp = fork();
 	assert(pipe(pipefd) != -1);
 	assert(pid_cp != -1);

 	if(pid_cp == 0){
 		close(pipefd[0]);
 		child_proc(pipefd[1], argc, argv, envp);
 		assert(0);

 	} else {
 		close(pipefd[1]);
 		parent_proc(pipefd[0]);
 		close(pipefd[0]);
 	}
}

void child_proc(int fd, int argc, char* argv[], char* envp[]){
	int ig = open("/dev/null", 0);
	dup2(ig, 1);
	dup2(ig, 2);

	char** strace_args = malloc(sizeof(char*)*(argc+2));
	strace_args[0] = "strace";
	strace_args[1] = "-T";
	for(int i=1; i<=argc; i++){
		strace_args[i+1] = argv[i];
	}

	char avai_path[512] = {0};
	strcpy(avai_path, getenv("PATH"));
	char full_path[100] = {0};
	char* tok_piece = strtok(avai_path, ":");
	strcpy(full_path, tok_piece);
	strcat(full_path, "/strace");
	printf("path %s\n", full_path);
	while((execve(tok_piece, strace_args, envp))==-1){
		tok_piece = strtok(NULL, ":");
		//assert(0);
		strcpy(full_path, tok_piece);
		strcat(full_path, "/strace");
		printf("full path%s", full_path);
	}
	assert(0);
}

void parent_proc(int fd){
	char ch;
	read(fd, &ch, 1);
	for(int i=0; i<10; i++){
		printf("shit %c", ch);
	}
	//while(1);
}






