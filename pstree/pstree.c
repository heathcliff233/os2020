#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/types.h>

const char* ROOT_DIR = "/proc/";
const char* MAX_PID_FILE = "/proc/sys/kernel/pid_max";

// To determine whether -n -p flag exist
int arg_n = 0;
int arg_p = 0;

typedef struct Process{
    char *comm; //The filename of the executable, in parentheses.
    int ppid;   //The PID of the parent of the process.
    int child;  //The child of the process.
    int next;   //The brother of the process.
    int is_root;//Whether it should be expanded, determine the style to print.
} Process;

Process* proc;  //Table of process information
pid_t pid_max;  //Index of the max PID

void init();
void scan_root();
void scan_task();
void add_child(int parent, int child);
void printProcess(int r);
void printParentProcess(int r);

int main(int argc, char *argv[]){

    // To preprocess args
    for (int i = 0; i < argc; i++){
        assert(argv[i]); // always true
        if (argv[i][0] == '-'){
            if (argv[i][1] == '-'){
                if (strcmp(argv[i], "--numeric-sort") == 0){
                    arg_n = 1;
                }else if (strcmp(argv[i], "--show-pids") == 0){
                    arg_p = 1;
                }else if (strcmp(argv[i], "--version") == 0){
                    fprintf(stderr, "pstree version 2.5 by 181240019\n");
                    return 0;
                }
            }else{
                for (int j = 1; j < strlen(argv[i]); j++){
                    if (argv[i][j] == 'n'){
                        arg_n = 1;
                    }else if (argv[i][j] == 'p'){
                        arg_p = 1;
                    }else if (argv[i][j] == 'V'){
                        fprintf(stderr, "pstree version 2.5 by 181240019\n");
                        return 0;
                    }
                }
            }
        }
    }
    assert(!argv[argc]); // always true

    init();
    scan_root();
    printProcess(1);

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

    proc = (Process *)malloc(sizeof(Process) * (pid_max + 1));
    for (int i = 0; i < pid_max; i++){
        proc[i].comm = NULL;
        proc[i].ppid = 0;
        proc[i].child = 0;
        proc[i].next = 0;
        proc[i].is_root = 0;
    }
}


void scan_root(){
    //Scan root processes.
    printf("in scan root\n");
    DIR* dir = opendir(ROOT_DIR);
    // TODO: handle exceptions
    struct dirent* p = NULL;
    while((p = readdir(dir)) != NULL){
        if((p->d_name[0] < '0')||(p->d_name[0] > '9')){
            continue;
        }

        char proc_dir[256];
        char proc_stat[256];
        snprintf(proc_dir, 256, "%s%s", ROOT_DIR, p->d_name); //Path of the directory
        snprintf(proc_stat, 256, "%s%s", proc_dir, "/stat");         //Path of the stat file

        int pid;
        char comm[256];  //The filename of the executable, in parentheses.
        char state[256]; //One of the characters, indicating process state
        int ppid;

        //Read the stat file to pid structure table
        FILE* st;
        st = fopen(proc_stat, "r");
        if(st == NULL){
            continue;
        }
        fscanf(st, "%d (%[^)]) %s %d", &pid, comm, state, &ppid); 
        // %[^)] is to match to the char before ')'

        fclose(st);
        // TODO: deal with special cases of PID 0, 1, 2
       /* 
        * PID0 is not in the directory
        * PID0 is the ppid of PID2
        * PID2 is the ppid of all system calls
        */
        if( ppid==2 || !(ppid<pid_max && pid<pid_max) ){
            continue;
        }
        int comm_len = strlen(comm) + 1;
        proc[pid].comm = (char*)malloc(sizeof(char)*(comm_len+10));
        strncpy(proc[pid].comm, comm, comm_len);
        proc[pid].comm[comm_len-1] = '\0';
        if(arg_p == 1){
            char pidStr[10] = "";
            sprintf(pidStr, "(%d)", pid);
            strncat(proc[pid].comm, pidStr, 10);
        }
        proc[pid].is_root = 1;
        if(pid==1) printf("pid=1\n");//("%s\n",proc[pid].comm);
        if(pid != 1){
            add_child(ppid, pid);
        }

        //Enter the task directory and scan tasks of the root
        char task_dir[256];
        snprintf(task_dir, 256, "%s%s", proc_dir, "/task");
        scan_task(task_dir, pid);

    }
    closedir(dir);

}

void scan_task(const char* curr_dir, int ppid){
    //Scan tasks of the root processes
    DIR* dir = opendir(curr_dir);
    if(!dir){
        return;
    }
    struct dirent* p = NULL;
    while((p = readdir(dir)) != NULL){
        char tp[256];
        char dir_proc[256];
        char stat_file[256];
        snprintf(tp, 256, "%s%s", curr_dir, "/");
        snprintf(dir_proc, 256, "%s%s", tp, p->d_name);
        snprintf(stat_file, 256, "%s%s", dir_proc, "/stat");
        FILE *pFile;
        pFile = fopen(stat_file, "r");

        if (pFile == NULL){
            continue;
        }

        int pid;
        char comm[256];
        fscanf(pFile, "%d (%[^)])", &pid, comm);
        fclose(pFile);

        if (
            !(pid < pid_max && ppid < pid_max) ||
            !(proc[pid].comm == NULL) ||
            (pid == ppid))
        {
            continue;
        }

        int comm_len = strlen(comm) + 1;
        proc[pid].comm = (char *)malloc(sizeof(char) * (comm_len));
        strncpy(proc[pid].comm, comm, comm_len);
        proc[pid].comm[comm_len - 1] = '\0';
        if(arg_p == 1){
            char pidStr[10] = "";
            sprintf(pidStr, "(%d)", pid);
            strncat(proc[pid].comm, pidStr, 10);
        }
        add_child(ppid, pid);

    }
    closedir(dir);

}

void add_child(int ppid, int pid){
    //Link the proc with binary tree
    proc[pid].ppid = ppid;
    int child = proc[ppid].child;
    if(child == 0){
        proc[ppid].child = pid;
    }else{
        if(arg_n == 1){
            if(pid < child){
                proc[pid].next = child;
                proc[ppid].child = pid;
            }else{
                while((proc[child].next!=0)&&(pid>proc[child].next)){
                    child = proc[child].next;
                }
                proc[pid].next = proc[child].next;
                proc[child].next = pid;
            }

        }else{
            proc[pid].next = child;
            proc[ppid].child = pid;
        }
    }

}

void printProcess(int p){//proc) {
  /* print (pid) to name */
  printf("%s%s%s", 
      (p==1 ? "":(p == proc[proc[p].ppid].child ? (proc[p].next!=0 ? "-+-" : "---") : (proc[p].next ? " |-" : " `-"))), 
      proc[p].comm, 
      proc[p].child!=0 ? "" : "\n");
  
  if (proc[p].child!=0) printProcess(proc[p].child);
  if (proc[p].next!=0) {
      printParentProcess(proc[p].ppid);
      printProcess(proc[p].next);
  }
}

void printParentProcess(int p) {
    /* Print the vertical lines of parent processes */
    if (proc[p].ppid!=0) printParentProcess(proc[p].ppid);
    printf("%s%*s",
      (p==1 ? "" : (proc->next ? " | " : "   ")),
      (int) strlen(proc[p].comm), "");
}

