#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <dlfcn.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>

void compile(char* path);
char src[32], out[32];
#if defined(__i386__)
	#define CC_ABI "-m32"
#elif defined(__x86_64__)
	#define CC_ABI "-m64"
#endif

void* handle;

int main(int argc, char *argv[]) {
  static char line[4096];
  int evaluate = 1;
  while (1) {
    printf("crepl> ");
    fflush(stdout);
    evaluate = 1;
    assert(fgets(line, sizeof(line), stdin));
    if(strlen(line)>3 && strncmp(line, "int ",4)){
    	evaluate = 0;
    }
    
    char tmp_file[32] = "./tmp/XXXXXX";
    int fd = mkstemp(tmp_file);

    if(evaluate){
    	write(fd, "int __expr_wrapper4(){return ", 29);
    	write(fd, line, strlen(line));
    	write(fd, ";}", 2);
    } else {
    	write(fd, line, strlen(line));
    }
    compile(tmp_file);
    unlink(tmp_file);
    handle = dlopen(out, RTLD_LAZY|RTLD_GLOBAL);
    if(evaluate){
    	printf("OK\n");
    } else {
    	int (*fun)(void) = dlsym(handle, "__expr_wrapper4");
    	printf("%d\n", fun());
    }
    unlink(out);
  }
  return 0;
}

char *cargv[] = {"gcc", "-x", "c", "-shared", "-o", CC_ABI, "-fPIC", out, src};

void compile(char* path){
	sprintf(out, "%s.so", path);
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
		}
	}
}
