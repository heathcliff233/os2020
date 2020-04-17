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
	
	while((execve(full_path, strace_args, envp))==-1){
		memset(full_path, '\0', 100);
		strcpy(full_path, strtok(NULL, ":"));
		strcat(full_path, "/strace");
	}
	assert(0);
}

static int readl(int fd, char* line){
	char ch;
	int ptr = 0;
	while(read(fd, &ch, 1) > 0){
		line[ptr] = ch;
		if(ch == '\n'){
			line[ptr] = '\0';
			return 1;
		} else if(ch == EOF){
			line[ptr] = '\0';
			return 0;
		}
		ptr++;
		/*
		if(ptr > 1022){
			printf("%s\n", line);
			assert(0);
		}
		*/
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
	char line[2048] = "";
	int wstatus = 0;
	
	sys_t call_list[1000];
	for(int i=0; i<1000; i++){
		strcpy(call_list[i].name, "NONE");
		call_list[i].time = 0;
	}
	char call_name[50] = "";
	double ex_time;
	double tot_time = 0;

	//int i = 0;
	//int len = 0;
	//int ptr = -1;
	time_t next_frame = time(NULL);
	int listLen = 0;
	
    while(readl(fd, line) > 0){
    	time_t pre = time(NULL);
		if(pre >= next_frame+1){
			//qsort(call_list, len, sizeof(sys_t), compare_list);
			qsort(call_list, listLen, sizeof(sys_t), compare_list);
			//printf("\033[2J\033[1;1H");
			for(int j=0; j<5; j++){
				printf("%s (%d%%)\n",call_list[j].name,(int)(call_list[j].time*100/tot_time));
			}
			for(int k=0; k<80; k++){
				printf("%c",'\0');
			}
			fflush(stdout);
			next_frame  = pre;
		}

		/*
		sscanf(line, "%[^(]%*[^<]<%lf>", call_name, &ex_time);	
		tot_time += ex_time;
		for(int t=0; t<len; t++){
			if(strcmp(call_name, call_list[t].name)==0){
				ptr = t;
				break;
			}
		}
		if(ptr == -1){
			strcpy(call_list[len].name, call_name);
			call_list[len].time = ex_time;
			len++;
		} else {
			call_list[ptr].time += ex_time;
		}
		ptr = -1;
		*/

		/*----------------------*/
		int len = strlen(line);
        int left = -1;
        int right = len-1;
        int leftparameter = -1;
        for (int i = len-1; i >= 0; i--) {
          if (line[i] == '<') {
            left = i;
            break;
          }
        }
        for (int i = 0; i < len; i++) {
          if (line[i] == '(') {
            leftparameter = i;
            break;
          }
        }
        if (line[right]=='>'){
          char time[100];
          char syscall[50];
          memset(syscall, '\0', 50);
          memset(time, '\0', 100);
          if (leftparameter > 0&&('a'<=line[0] && 'z'>=line[0])){
            memcpy(time, &line[left+1], (right-left-1));
            memcpy(syscall, &line[0], leftparameter);
            double dtime = strtod(time, NULL);
            for (int i = 0; i < len; i++) {
              if (strcmp(call_list[i].name, "NONE") != 0) {
                if (strcmp(call_list[i].name, syscall) == 0) {
                  call_list[i].time += dtime;
                  tot_time += dtime;
                  break;
                }
              }
              if (strcmp(call_list[i].name, "NONE") == 0) {
                listLen += 1;
                strcpy(call_list[i].name, syscall);
                call_list[i].time = dtime;
                tot_time += dtime;
                assert(call_list[i].name != NULL);
                break;
              }
            }
          }
        }
        //memset(line, '\0', sizeof(line));
		/*----------------------*/
		//qsort(call_list, len, sizeof(sys_t), compare_list);
        //qsort(call_list, listLen, sizeof(sys_t), compare_list);
		memset(line, '\0', sizeof(line));

	}
	qsort(call_list, listLen, sizeof(sys_t), compare_list);
	for(int j=0; j<5; j++){
		printf("%s (%d%%)\n",call_list[j].name,(int)(call_list[j].time*100/tot_time));
	}
	
	for(int k=0; k<80; k++){
		printf("%c",'\0');
	}
	fflush(stdout);
	
}






