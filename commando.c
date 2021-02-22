#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "commando.h"

int main(int argc, char* argv[]){
    setvbuf(stdout, NULL, _IONBF, 0);                   // Turn off output buffering
    char arr[MAX_LINE + 1];                             // array to hold line that user inputs
    char* tokens[ARG_MAX + 1];                          // array to hold tokens created through parse_into_tokens 
    cmdcol_t *newCmdCol = malloc(sizeof(cmdcol_t)); 
    newCmdCol->size = 0;  
    if(argc > 1){                                       // check if --echo option is turned on 
        if(strcmp(argv[1],"--echo") == 0){
            setenv("COMMANDO_ECHO","1",1); 
        }
    }
    while(1){
        int idx = 0; 
        int *numTok = malloc(sizeof(int));              // variable passed into the parse_into_tokens function 
        printf("@> "); 
        char* getLine = fgets(arr,MAX_LINE,stdin);
        if(getLine == NULL){                            // detect if end of line is reached 
            printf("\nEnd of input"); 
            free(numTok);
            break; 
        } 
        parse_into_tokens(arr,tokens,numTok);           
        if(getenv("COMMANDO_ECHO") != NULL){            // if echo option is turned on print back out user input 
            while(tokens[idx] != NULL){
                printf("%s ",tokens[idx]);
                idx++; 
            } 
            printf("\n");
        }
        if(tokens[0] != NULL){                         // the user actually typed in input (if user did not just click enter)
            if(strcmp(tokens[0],"help") == 0){
                printf("COMMANDO COMMANDS\n"); 
                printf("help                : show this message\n");
                printf("exit                : exit the program\n");
                printf("list                : list all jobs that have been started giving information on each\n");
                printf("pause nanos secs    : pause for the given number of nanseconds and seconds\n");
                printf("output-for int      : print the output for given job number\n");
                printf("output-all          : print output for all jobs\n");
                printf("wait-for int        : wait until the given job number finishes\n");
                printf("wait-all            : wait for all jobs to finish\n");
                printf("command arg1 ...    : non-built-in is run as a job\n");
            }else{
                if(strcmp(tokens[0],"exit") == 0){              // break out from while loop and go to end of main to free things that need to be free'd 
                    free(numTok);
                    break;
                }else{
                    if(strcmp(tokens[0],"list") == 0){          // print out processes 
                        cmdcol_print(newCmdCol); 
                        cmdcol_update_state(newCmdCol,NOBLOCK); 
                    }else{
                        if(strcmp(tokens[0],"pause") == 0){                 // use the pause_for function to pause for user given amount of time 
                            pause_for(atoi(tokens[1]),atoi(tokens[2]));
                            cmdcol_update_state(newCmdCol,NOBLOCK); 
                        }else{
                            if(strcmp(tokens[0],"output-for") == 0){
                                printf("@<<< Output for %s[#%d] (%d bytes):\n",newCmdCol->cmd[atoi(tokens[1])]->name,
                                newCmdCol->cmd[atoi(tokens[1])]->pid,newCmdCol->cmd[atoi(tokens[1])]->output_size);
                                printf("----------------------------------------\n");
                                cmd_print_output(newCmdCol->cmd[atoi(tokens[1])]);                  // this line calls the cmd_print_output function that actually prints out output of process
                                printf("----------------------------------------\n");
                                cmdcol_update_state(newCmdCol,NOBLOCK); 
                            }else{
                                if(strcmp(tokens[0],"output-all") == 0){
                                    for(int i = 0; i < newCmdCol->size;i++){
                                        printf("@<<< Output for %s[#%d] (%d bytes):\n",newCmdCol->cmd[i]->name,
                                        newCmdCol->cmd[i]->pid,newCmdCol->cmd[i]->output_size);
                                        printf("----------------------------------------\n");
                                        cmd_print_output(newCmdCol->cmd[i]);                       // same thing as output-for but this line is inside a for loop so that all process output are printed out 
                                        printf("----------------------------------------\n");
                                    }
                                    cmdcol_update_state(newCmdCol,NOBLOCK); 
                                }else{
                                    if(strcmp(tokens[0],"wait-for") == 0){
                                        cmd_update_state(newCmdCol->cmd[atoi(tokens[1])],DOBLOCK);          // use DOBLOCK so that commando actually waits 
                                    }else{
                                        if(strcmp(tokens[0],"wait-all") == 0){
                                            for(int j = 0; j < newCmdCol->size;j++){
                                                cmd_update_state(newCmdCol->cmd[j],DOBLOCK);                // use DOBLOCK inside for loop so that commando actually waits for all processes 
                                            }
                                        }else{
                                            cmd_t *newCmd = cmd_new(tokens);                      // if none of the COMMANDO COMMANDS are given then fork a new process 
                                            cmd_start(newCmd);                                    // set up the pipe 
                                            cmdcol_add(newCmdCol,newCmd);                         // add new cmd to the cmdcol structure that we initialized at the start
                                            cmdcol_update_state(newCmdCol,NOBLOCK);               // update all processes in the cmdcol structure 
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        free(numTok);                   // free mallooc'd integer variable we used for parse_into_tokens                                                                        
    }   // end of loop 
    cmdcol_freeall(newCmdCol);          // free all cmd structures inside cmdcol->cmd field 
    free(newCmdCol);                    // free malloc'd cmdcol structure 
}
