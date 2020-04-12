#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
/*
int main(int argc, char* argv[]){
 	char *exec_argv[] = { "strace", "ls", NULL, };
 	char *exec_envp[] = { "PATH=/bin", NULL, };
 	//execve("strace",          exec_argv, exec_envp);
 	//execve("/bin/strace",     exec_argv, exec_envp);
 	execve("/usr/bin/strace", exec_argv, exec_envp);
 	perror(argv[0]);
 	exit(EXIT_FAILURE);
}
*/

void child_proc(int* fd, int argc, char* argv[], char* envp[]);
void parent_proc(int fd);

int main(int argc, char* argv[], char* envp[]) {
 	int pid_cp = -1;
 	int pipefd[2] = {};
 	assert(pipe(pipefd) != -1);
	pid_cp = fork();
 	assert(pid_cp != -1);

 	if(pid_cp == 0){
 		close(pipefd[0]);
		dup2(pipefd[1], 2);
 		child_proc(pipefd, argc, argv, envp);
 		assert(0);

 	} else {
 		close(pipefd[1]);
 		parent_proc(pipefd[0]);
 		close(pipefd[0]);
 	}
}

void child_proc(int* fd, int argc, char* argv[], char* envp[]){
	int ig = open("/dev/null", 0);
	dup2(ig, 1);
	//dup2(fd[1], 2);

	char** strace_args = malloc(sizeof(char*)*(argc+2));
	strace_args[0] = "strace";
	strace_args[1] = "-T";
	for(int i=1; i<=argc; i++){
		strace_args[i+1] = argv[i];
	}

	char* avai_path_cp = getenv("PATH");
	char avai_path[512];
	strcpy(avai_path, avai_path_cp);
	char full_path[100];
	memset(full_path, '\0', 100);
	char* tok_piece = strtok(avai_path, ":");
	strcpy(full_path, tok_piece);
	strcat(full_path, "/strace");
	printf("path %s\n", full_path);
	
	while((execve(full_path, strace_args, envp))==-1){
		//perror("shit not this");
		//tok_piece = strtok(NULL, ":");
		//assert(0);
		//printf("path %s\n", full_path);
		memset(full_path, '\0', 100);
		//strcpy(full_path, tok_piece);
		strcpy(full_path, strtok(NULL, ":"));
		strcat(full_path, "/strace");
		printf("%s\n", full_path);
	}
	assert(0);
}

static int readl(int fd, char* line){
	char ch;
	int ptr = 0;
	while(read(fd, &ch, 1) > 0){
		line[ptr] = ch;
		//printf("%c", ch);
		if(ch == '\n'){
			line[ptr] = '\0';
			//printf("line\n");
			return 1;
		} else if(ch == EOF){
			line[ptr] = '\0';
			return 0;
		}
		ptr++;
	}
	return -1;
}

typedef struct syscallStruct{
	char name[50];
	double time;
} sys_t;

int compare_list(const void* p1, const void* p2){
	return ((*(sys_t *)p1).time > (*(sys_t *)p2).time)?-1:1;
}

void parent_proc(int fd){
/*	
	char ch;
	sleep(5);
	read(fd, &ch, 1);
	for(int i=0; i<1000; i++){
		printf("%c", ch);
	}
	printf("\n");
	//while(1);
	*/
	
	printf("in father proc\n");	
	char line[1024] = "";
	int wstatus = 0;
	time_t next_frame = time(NULL);
	sys_t call_list[1000];
	char call_name[50] = "";
	double ex_time;

	int i = 0;
	int len = 0;
	int ptr = -1;
	while(waitpid(-1, &wstatus, WNOHANG) == 0 && readl(fd, line) >= 0){
		/*
		if(call_list[i].name[0] == 0){
			sscanf(line, "%[^(]%*[^<]<%lf>", call_list[i].name, &(call_list[i].time));
		} else {
			i++;
			sscanf(line, "%*[<]<%lf>", &(call_list[i].time));
			call_list[i].name[0] = 0;
		}
		//printf("shit %s \n", call_list[i].name);
*/
		sscanf(line, "%[^(]%*[^<]<%lf>", call_name, &ex_time);	
		//printf("scanned %s %f\n", call_name, ex_time);
		for(int t=0; t<len; t++){
			if(strcmp(call_name, call_list[t].name)==0){
				ptr = t;
				break;
			}
		}
		if(ptr == -1){
			strcpy(call_list[len].name, call_name);
			call_list[len].time = ex_time;
			printf("found %s %f\n", call_list[len].name, call_list[len].time);
			len++;
		} else {
			printf("same\n");
			call_list[ptr].time += ex_time;
		}

		qsort(call_list, len, sizeof(sys_t), compare_list);
		if(time(NULL) > next_frame){
			next_frame += 1;
			for(int j=0; j<5; j++){
				printf("%s time %d\n",call_list[i].name,(int)(call_list[i].time));
			}
			for(int k=0; k<80; k++){
				printf("%c",'\0');
			}
			fflush(stdout);
		}
	}
	printf("lastttttttttt\n name %s\n", call_list[i].name);
	/*
	for(int j=0; j<i; j++){
		printf("%s time %d\n",call_list[i].name,(int)(call_list[i].time));
	}
	*/
	for(int k=0; k<80; k++){
		printf("%c",'\0');
	}
	fflush(stdout);

	
	
	
}






