#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/types.h>

const char* ROOT_DIR = "/proc/";
const char* MAX_PID_FILE = "/proc/sys/kernel/pid_max";

// To determine whether -n -p flag exist
int arg_n = 0;
int arg_p = 0;

/*
int main(int argc, char *argv[]) {
  for (int i = 0; i < argc; i++) {
    assert(argv[i]);
    printf("argv[%d] = %s\n", i, argv[i]);
  }
  assert(!argv[argc]);
  return 0;
}
*/


typedef struct Process
{
    char *comm; //The filename of the executable, in parentheses.
    pid_t ppid; //The PID of the parent of the process.
    int son;    //The son of the process.
    int next;   //The brother of the process.
    int is_root;//Whether it should be expanded, determine the style to print.
} Process;

Process* proc;  //Table of process information
int root;       //Root of all process
pid_t pid_max;  //Index of the max PID

char tabs[1024];
int tabs_ptr = 0;
int* list;

void init();
void scan_root();
void scan_task();
int is_digit(char* s);
void add_child(int parent, int child);
void tree_sort(int root);
void print_tree(int root);

int main(int argc, char *argv[]){

    // To preprocess args
    for (int i = 0; i < argc; i++){
        assert(argv[i]); // always true
        // printf("argv[%d] = %s\n", i, argv[i]);
        if (argv[i][0] == '-'){
            if (argv[i][1] == '-'){
                if (strcmp(argv[i], "--numeric-sort") == 0){
                    arg_n = 1;
                }else if (strcmp(argv[i], "--show-pids") == 0){
                    arg_p = 1;
                }else if (strcmp(argv[i], "--version") == 0){
                    printf("pstree for OSLab M1 by 181240019\nfuck you OJ\n");
                    return 0;
                }
            }else{
                for (int j = 1; j < strlen(argv[i]); j++){
                    if (argv[i][j] == 'n'){
                        arg_n = 1;
                    }else if (argv[i][j] == 'p'){
                        arg_p = 1;
                    }else if (argv[i][j] == 'V'){
                        printf("pstree for OSLab M1 by 181240019\nfuck you OJ\n");
                        return 0;
                    }
                }
            }
        }
    }

    assert(!argv[argc]); // always true

    // printf("n: %d, p: %d\n", arg_n, arg_p);

    init();
    scan_root();
    memset(tabs, 0, sizeof(tabs_ptr));
    print_tree(root);

    free(list);
    free(proc);
    return 0;
}

void init(){
    // Initialize the table to store proc information 
    FILE *pFile;
    pFile = fopen(MAX_PID_FILE, "r");
    assert(pFile);
    fscanf(pFile, "%d", &pid_max); // Get the max pid
    fclose(pFile);

    list = (int *)malloc(sizeof(int) * ((int)pid_max + 1));
    memset(list, 0, sizeof(int) * ((int)pid_max + 1));

    proc = (Process *)malloc(sizeof(Process) * (pid_max + 1));
    for (int i = 0; i < pid_max; i++){
        proc[i].comm = NULL;
        proc[i].ppid = 0;
        proc[i].son = 0;
        proc[i].next = 0;
        proc[i].is_root = 0;
    }
}

int is_digit(char* s){
    //Use library function to improve robustness
    while(isdigit(*s-'0')){
        ++s;
    }
    return *s=='\0';
}

void scan_root(){
    //Scan root processes for pstree display with no args.
    DIR* dir = opendir(ROOT_DIR);
    // TODO: handle exceptions
    struct dirent* p = NULL;
    while((p = readdir(dir)) != NULL){
        if(!is_digit(p->d_name)){
            continue;
        }
    }
    // TODO: not finished

}


