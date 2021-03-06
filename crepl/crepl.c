#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <dlfcn.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>

void compile(char* path);
char src[128], out[128];
#if defined(__i386__)
	#define CC_ABI "-m32"
#elif defined(__x86_64__)
	#define CC_ABI "-m64"
#endif

char *cargv[] = {"gcc", "-fPIC", CC_ABI, "-shared", "-x", "c", "-o", out, src, NULL};

void* handle;
int flag;

int main(int argc, char *argv[]) {
  static char line[4096];
  int evaluate = 1;
  while (1) {
    printf("crepl> ");
    fflush(stdout);
    flag = 1;
    evaluate = 1;
    //assert(fgets(line, sizeof(line), stdin));
    if(!fgets(line, sizeof(line), stdin)){
    	break;
    }
    if(strlen(line)>3 && strncmp(line, "int ",4)==0){
    	evaluate = 0;
    }
    
    char tmp_file[128] = "/tmp/XXXXXX";
    int fd = mkstemp(tmp_file);
    if(fd<0)printf("fail to create tmp\n");
    if(evaluate == 1){
    	write(fd, "int __expr_wrapper4(){return ", 29);
    	write(fd, line, strlen(line));
    	write(fd, ";}\n", 3);
    } else {
    	write(fd, line, strlen(line));
    }
    compile(tmp_file);
    if(flag==0)continue;
    handle = dlopen(out, RTLD_LAZY|RTLD_GLOBAL);
    /*
    if(handle==NULL){
    	printf("load failed\n");
    	continue;
    }
    */
    if(evaluate == 0){
    	printf("OK\n");
    } else {
    	int (*fun)(void) = dlsym(handle, "__expr_wrapper4");
    	printf("%d\n", fun());
    	unlink(out);
    }
    
  }
  return 0;
}


void compile(char* path){
	sprintf(out, "%s.so", path);
	strcpy(src, path);
	int ppid = fork();
	if(ppid == 0){
		close(1);
		close(2);
		execvp("gcc",cargv);
		assert(0);
	} else {
		int wstatus = 0;
		wait(&wstatus);
		if(wstatus!=0){
			printf("Compile error\n");
			flag = 0;
		}
	}
}

