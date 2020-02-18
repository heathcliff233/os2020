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


typedef struct Process{
    char *comm; //The filename of the executable, in parentheses.
    int ppid; //The PID of the parent of the process.
    int child;    //The child of the process.
    int next;   //The brother of the process.
    int is_root;//Whether it should be expanded, determine the style to print.
} Process;

Process* proc;  //Table of process information
pid_t pid_max;  //Index of the max PID

char tabs[1024];
int tabs_ptr = 0;
//int* list;

void init();
void scan_root();
void scan_task();
int is_digit(char* s);
void add_child(int parent, int child);
//void tree_sort(int root);
//int tree_cmp(int p1, int p2);
//void print_tree(int root);
void printProcess(int r);
void printParentProcess(int r);

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
    memset(tabs, 0, sizeof(tabs_ptr));  //TODO: fix it! Segmentation fault
    //print_tree(1);
    printf("start print\n");
    printProcess(1);

    //free(list);
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

    //list = (int *)malloc(sizeof(int) * ((int)pid_max + 1));
    //memset(list, 0, sizeof(int) * ((int)pid_max + 1));

    proc = (Process *)malloc(sizeof(Process) * (pid_max + 1));
    for (int i = 0; i < pid_max; i++){
        proc[i].comm = NULL;
        proc[i].ppid = 0;
        proc[i].child = 0;
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
    //Scan root processes.
    DIR* dir = opendir(ROOT_DIR);
    // TODO: handle exceptions
    struct dirent* p = NULL;
    while((p = readdir(dir)) != NULL){
        if(!is_digit(p->d_name)){
            continue;
        }

        char tmp[256];
        char proc_dir[256];
        char proc_stat[256];
        snprintf(tmp, 256, "%s%s", ROOT_DIR, "/");
        snprintf(proc_dir, 256, "%s%s", tmp, p->d_name); //Path of the directory
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
        if( ppid==0 || ppid==2 || !(ppid<pid_max && pid<pid_max) ){
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
        add_child(ppid, pid);

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
        if(pid==1) printf("%s",proc[pid].comm);
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
      (p==1 ? "shit":(p == proc[proc[p].ppid].child ? (proc[p].next!=0 ? "-+-" : "---") : (proc[p].next ? " |-" : " `-"))), 
      proc[p].comm, 
      proc[p].child!=0 ? "ass" : "\n");
  
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

/*
void print_tree(int r){
    //Print the proc structure
    if (r == 0){
        return;
    }

    int tabs_ptr_backup = tabs_ptr;

    if (r != 1){
        printf("-");
        tabs_ptr += 1;
    }
    if (proc[r].is_root == 0){ 
        // have no child, print within {}
        printf("{%s}", proc[r].comm);
        tabs_ptr += (strlen(proc[r].comm) + 2);
    }else{
        printf("%s", proc[r].comm);
        tabs_ptr += strlen(proc[r].comm);
    }
    if (arg_p == 1){
        printf("(%d)", r);
        char pid_str[32];
        sprintf(pid_str, "%d", r);
        tabs_ptr += (2 + strlen(pid_str));
    }
    if (proc[r].child != 0){
        if (proc[proc[r].child].next != 0){
            tabs[tabs_ptr + 1] = 1;
            printf("-+");
        }else{
            tabs[tabs_ptr + 1] = 0;
            printf("--");
        }
        tabs_ptr += 2;
        // tabs[tabs_ptr - 1] = 1;
        print_tree(proc[r].child);
        int son_ptr = proc[proc[r].child].next;
        while (son_ptr != 0){
            for (int i = 0; i < tabs_ptr - 1; i++){
                if (tabs[i] == 0){
                    printf(" ");
                }
                else{
                    printf("|");
                }
            }
            if (proc[son_ptr].next != 0){
                printf("|");
            }else{
                tabs[tabs_ptr - 1] = 0;
                printf("`");
            }
            print_tree(son_ptr);
            son_ptr = proc[son_ptr].next;
        }
    }
    else{
        printf("\n");
    }
    for (int i = tabs_ptr_backup; i < tabs_ptr; i++){
        tabs[i] = 0;
    }
    tabs_ptr = tabs_ptr_backup;
    return;
}
*/
