#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <ctype.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <poll.h>
#include <signal.h>
#include <errno.h>

/* Authors: Saw Lin'17 & Carlton Yang'17
Collaboration note: we both work on the entire project: sometimes we take turns or work together on coding different functions, debugging the shell, and dealing with special cases. 
By the way, for strings the user type in in the command line, we didn't deal with it. For example, /bin/echo "blah blah blah" will be tokenified into {"/bin/echo",""blah","blah","blah"",NULL}. 
*/

//record type to keep track of info about processes
struct process{
	pid_t pppid;
	char command[1024];
	char state[128];
	struct process *next;
};

//adding struct process to a linkedlist
void process_add(pid_t pid, char* command, char* state, struct process **process_list) {
	if ((*process_list) == NULL) {
		struct process *new_pro = (struct process *)malloc(sizeof(struct process)); 
		new_pro->pppid = pid;
		strcpy(new_pro->command,command);
    		strcpy(new_pro->state,state);
		new_pro->next = NULL;
		*process_list = new_pro;
	}
	else {
		struct process *head = *process_list;
		while (head->next != NULL) {
			head = head->next; 
		}
		struct process *new_pro = (struct process *)malloc(sizeof(struct process));
		strcpy(new_pro->command,command);
    		strcpy(new_pro->state,state);
    		new_pro->pppid = pid;
    		new_pro->next = NULL;
    		head->next = new_pro;
	}
}

//when a process is finished, delete from linkedlist of processes we need to check
void delete_process(pid_t pid, struct process **list) {
    	struct process *temp = *list;
    	if(temp != NULL) {
        	if (temp->pppid == pid && temp->next == NULL){
            		*list = (*list)->next;
            		free(temp);
            		return;
        	}

        	while(temp->next != NULL){
			if ((temp->next)->pppid == pid){
                		struct process *temp2 = temp->next;
                		temp->next = (temp->next)->next;
                		free(temp2);
                		return;
			}
            		temp = temp->next;

		}
	}    
}

//print each job when 'jobs' is called
void process_print(struct process *head) {
    //struct process *current=
    if (head ==NULL){
		printf("No process is currently running.\n");
	}
    else{
   	 while (head != NULL) 
	{
        	printf("Process ID: %d, command: %s, state: %s\n", head->pppid, head->command, head->state);
        	head = head->next;
    	}
    }
}


//free the process list from heap
void processlist_clear(struct process *list) {
    while (list != NULL) {
        struct process *tmp = list;
        list = list->next;
        free(tmp);
    }
}

//check argument pid is valid or not
int check_pid(pid_t pid,struct process *head,char *state)
{
	//traverse the linked list
	while(head != NULL)
	{
		//if match with process list, then return 1(true)
		if (head->pppid == pid) {
			strcpy(head->state,state);
			return 1;
		}
		head = head->next;
	//else return 0(false)
	}
	return 0;
}

//record type for keeping track of file paths
struct config {
	char path[1024];
	struct config *next;
};

//adding paths to a linkedlist
void path_add(char *path, struct config **list) {
	if ((*list) == NULL) {
		struct config *new = malloc(sizeof (struct config));
		strcpy(new->path, path); 
		new->next = NULL; 
		*list = new; 
	}
	else {
		struct config *head = *list; 
		while (head->next != NULL) {
			head = head->next; 
		}
		struct config *new = malloc(sizeof (struct config));
		strcpy(new->path, path); 
		new->next = NULL; 
		head->next = new; 
	}
}

//free path list from heap
void pathlist_clear(struct config *list) {
    while (list != NULL) {
        struct config *tmp = list;
        list = list->next;
        free(tmp);
    }
}

//print each path in the list for paths
void list_print(struct config *head) {
    while (head != NULL) {
        printf("Node at address %p has value %s\n", head, head->path);
        head = head->next;
    }
}

//tokenify from lab (parameter modified) 
char** tokenify(const char *s, const char *om) {
    char *sdu = strdup(s);
    char *sdu2 = strdup(s);
    int count = 0;
    char *token; 
    for (token = strtok(sdu,om); token!=NULL; token = strtok(NULL,om)){
	count++; 
    }
    char **arr = malloc((count+1)*sizeof(char*));
    int index =0;
    for (token = strtok(sdu2,om); token!=NULL; token = strtok(NULL,om)){
	arr[index] = strdup(token);
	index++; 	
    }
    arr[count]=NULL;
    free(sdu);
    free(sdu2);
    return arr;  
}

void free_tokens(char **tokens) {
    int i = 0;
    while (tokens[i] != NULL) {
        free(tokens[i]); 
        i++;
    }
    free(tokens);
}

void print_tokens(char *tokens[]) {//has to be deleted eventually 
    int i = 0;
    while (tokens[i] != NULL) {
        //printf("Token %d: %s\n", i+1, tokens[i]);
	printf("Token (cmd): %s\n", tokens[i]);
        i++;
    }
}

//print mode for command mode
void print_mode (int *mode)
{
	if (*mode==1)
	{
		printf("This is parallel mode\n");
	}
	else
	{
		printf("This is sequential mode\n");
	}
}

//find explicitpath for command (concat the paths read from a file and the user input command)
char *explicitpath (struct config **list,char *enter) {
	char *new=(char *)malloc((strlen((*list)->path)+1)*sizeof(char)+strlen(enter));//create exact size
	strcpy(new,(*list)->path);
	strcat(new, enter);
	return new;
}

//see if the path entered is a valid one
char *path_valid(struct config **reading_list,char *enter) {
	struct config *current = *reading_list;
	struct stat statresult;
	int rv = stat(enter, &statresult); //see if file exist 
	if (rv < 0) {	// stat failed; file definitely doesn't exist; now try to prepend each directory from file
		while (current!=NULL)
		{
			struct stat statresult2;
			char *newpath = explicitpath (&current,enter);	//prepend path from path list
			int rv2= stat(newpath,&statresult2);		//see if new path is valid
			if (rv2==0)		//if yes,return new path
			{
				free(enter);	//this is essentially the cmd[0], if we dont free it, its lost
				return newpath;	
			}
			free(newpath);		//free newpath everytime
			current=current->next;
		}
	}
	else if(rv>0)
	{	
		printf("Error in checking valid path: %s\n", strerror(errno));
	}
	//else, return the original path(not changed by the function)
	return enter;
}

//execute commands
void execute_cmd (char *tokens[], int *mode, int *status, struct config **list, struct process **process_list){
	int i = 0;
	int childrv[1024] = {0}; 
	char state[128];
	while (tokens[i] != NULL) {
		char** cmd = tokenify(tokens[i]," \r\t\n");
		
		//print_tokens(cmd); 
		if (cmd[0] == NULL) {//takes care of empty command line
			i++;
			continue;
		}
		//pause command
		else if (strcasecmp(cmd[0],"pause")== 0) {
			strcpy(state,"paused");
			pid_t pid = atoi(cmd[1]);
			if (check_pid(pid,*process_list,state)) { //check argument-pid
				kill(pid,SIGSTOP);
			}
			else {printf("Invalid Pid.\n");}
			i++; 
			continue;
		}
		//resume command
		else if (strcasecmp(cmd[0],"resume")== 0) {
			strcpy(state,"running");
			pid_t pid = atoi(cmd[1]);
			if (check_pid(pid,*process_list,state)) {//check argument-pid
				kill(pid,SIGCONT);
			}
			else {printf("Invalid Pid.\n");}
			i++; 
			continue;
		}
		//print jobs
		else if (strcasecmp(cmd[0],"jobs")== 0) {
			if (cmd[1]==NULL){
				process_print(*process_list);
			}
			else {
				printf("Invalid arguments. Do not pass anything to jobs.\n");
				free_tokens(cmd);	//prevent memory leak
			}
			i++; 
			continue;
		}
		if (strcasecmp(cmd[0],"mode")!= 0 && strcasecmp(cmd[0],"exit")!= 0) {//if the commands are neither "mode" nor "exit"
			cmd[0]=path_valid(list,cmd[0]);		//check if path/command is valid
			childrv [i] = fork();
			if (childrv [i] == 0) {
				if (execv(cmd[0], cmd) < 0) {
        				fprintf(stderr, "execv failed: %s\n", strerror(errno));
					*status = 0; //to handle invalid command: if execv gives error, then set status = 0, which means, it exits the child process  
    				}
    			}
			else {
				if (*mode == 0) {
					wait(&(childrv[i])); 
				}
				else {
					process_add(childrv[i],tokens[i],"running",process_list);
				}
			}
		}
		else if (strcasecmp(cmd[0],"mode")== 0) {//switching mode
			if (cmd[1] == NULL) {	//print current mode when only mode is typed	
				print_mode(mode);
				i++;
				continue;
			}
			else {
				if (cmd[1][0] == 'p' || (strcasecmp(&cmd[1][0],"parallel")== 0)) {
					*mode = 1; 
				}
				else if (cmd[1][0] == 's' ||(strcasecmp(&cmd[1][0],"sequential")== 0)) {
					*mode = 0; 
				}
				else {
					printf("Wrong argument for mode. Please use p (parallel) or s(sequential).\n");
					i++;
					free_tokens(cmd);
					continue;
				}
			}
		}
		else {//exit
			//traverse through process_list to see if there's any process still running; if there's one, print error, else, change *status = 0; 
			struct process *h = *process_list;
			int count = 0; 
			while (h != NULL) {
				if (kill(h->pppid,0) == 0){
					count++; 
				}
				h = h->next; 
			}
			if (count > 0) {
				printf("Cannot exit the shell; Please check the following processes; some are still running...\n"); 
				process_print(*process_list);
			}
			else {
				*status = 0; 
			}
		}
		free_tokens(cmd);
		i++; 	
	}
	i = 0; 
	//code for wait (stage 1, parallel mode) 
	/*if (*mode == 1) {
		while (tokens[i]!= NULL) {
			wait(&childrv[i]);
			i++; 
		}
	}*/	 
}

char *read_line (int *status) {
	char *buffer = malloc(sizeof(char) *1024);
	if (fgets(buffer,1024,stdin) != NULL){
		for (int i=0; i< strlen(buffer);i++){
			if (buffer[i] == '#'){
				buffer[i] = '\0';
			}
		}
	}
	else {//which means we've encountered an EOF
		*status = 0;//change status to 0, which means exit the shell loop  
		printf("\n");  
	}
	return buffer;
} 

void load_paths (struct config **head,const char *filename) {
	FILE *fl;	
	fl=fopen(filename,"r");
	char str[1024];
	while (fgets(str,1024,fl)!=NULL) {
		char *p = strchr(str,'\n');     //to strip of the newline character
		if (p) {
			*p ='\0';
		}
		path_add(str,head); 
	}  
	fclose(fl);
}

int *check_process(struct process **head,int *prompt) {
	struct process *current = *head;
	int status=0;
	while(current != NULL){
		status = waitpid(current->pppid, NULL, WNOHANG);
		if (status != 0){
			char *p = strchr(current->command,'\n');     //to strip of the newline character
			if (p) {
				*p ='\0';
			}
			//if (current->command[strlen(current->command)-1] == '\n')
			
			printf("Process ID: %d, Command: %s is completed.\n", current->pppid, current->command);
			fflush(stdout);	
			delete_process(current->pppid,head);
			*prompt=0;
		}

		current = current->next;
	}
	return prompt;
}

void shell_loop () {
	int status = 1; //will change to 0 when exit command is found
	int mode = 0; //0 - for sequential, 1 - for parallel
	struct config *readin_file = NULL; 
	struct process *process_list = NULL;
	load_paths(&readin_file, "shell-config"); 
	int prompt = 0; //when 1, we ask for prompt, when 0, do not ask for input(avoid repeating prompt)
	do {
		// declare an array of a single struct pollfd
		struct pollfd pfd[1];
	    	pfd->fd = 0; // stdin is file descriptor 0
	    	pfd->events = POLLIN;
	    	pfd->revents = 0;
 
 	    	// wait for input on stdin, up to 1000 milliseconds
 	    	int rv = poll(&pfd[0], 1, 100);
 	
 	   	// the return value tells us whether there was a 
 	   	// timeout (0), something happened (>0) or an
 	   	// error (<0).
		if (!prompt) {
			printf("prompt> ");
			fflush(stdout);
			prompt = 1;
		}
 	   	if (rv == 0) {	//check process when timeout 
			prompt = *check_process(&process_list, &prompt);
 	   	} 
		else if (rv > 0) {
 	        //handle the command
		
			char *buf = read_line(&status); 
			char** cmd_semicolon = tokenify(buf,";");
		
			execute_cmd (cmd_semicolon, &mode, &status, &readin_file, &process_list); 

			free_tokens(cmd_semicolon);
			free(buf); 
			prompt = 0;
	
 	   	} 
		else {
 		       printf("there was some kind of error: %s\n", strerror(errno));
 	   	}
	}while(status);
	pathlist_clear(readin_file); 
	processlist_clear(process_list);  
}

int main(int argc, char **argv) {
	shell_loop(); 	
    return 0;
}

