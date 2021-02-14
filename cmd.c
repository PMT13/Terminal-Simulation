#include <stdlib.h>
#include "commando.h"
#include <unistd.h>

cmd_t *cmd_new(char *argv[]){ 
    cmd_t *new = malloc(sizeof(cmd_t));
    int i = 0; 
    int x = 0; 
    for(int a = 0; a < ARG_MAX;a++){                // initialize all the indeces in new->argv
        new->argv[a] = NULL; 
    }
    for(int b = 0; b < NAME_MAX;b++){               // initialize all the indeces in new->name
        new->name[b] = '\0'; 
    }
    while(argv[i] != NULL){
        new->argv[i] = strdup(argv[i]);          // copies all the string arguments in argv parameter into cmd_t argv field
        i++; 
    }
    new->argv[ARG_MAX] = "\0";                      // ends the argv array with a null termination  
    while(argv[0][x] != '\0'){                      // gets the command 
        new->name[x] = argv[0][x]; 
        x++; 
    }                   
    snprintf(new->str_status,STATUS_LEN,"INIT");    
    new->finished = 0; 
    new->pid = -1; 
    new->status = -1; 
    new->output = NULL; 
    new->output_size = -1; 
    new->out_pipe[0] = 0; 
    new->out_pipe[1] = 0; 
    return new; 
}
// Allocates a new cmd_t with the given argv[] array. Makes string
// copies of each of the strings contained within argv[] using
// strdup() as they likely come from a source that will be
// altered. Ensures that cmd->argv[] is ended with NULL. Sets the name
// field to be the argv[0]. Sets finished to 0 (not finished yet). Set
// str_status to be "INIT" using snprintf(). Initializes the remaining
// fields to obvious default values such as -1s, and NULLs.

void cmd_free(cmd_t *cmd){
    int i = 0; 
    while(cmd->argv[i] != NULL){
        free(cmd->argv[i]);
        i++;
    }
    if(cmd->output != NULL){
        free(cmd->output);
    }
    free(cmd); 
}
// Deallocates a cmd structure. Deallocates the strings in the argv[]
// array. Also deallocats the output buffer if it is not
// NULL. Finally, deallocates cmd itself.

void cmd_start(cmd_t *cmd){
    if(pipe(cmd->out_pipe) < 0){
        printf("pipe error"); 
    }
    pid_t child = fork();
    if(child < 0){
        printf("fork error");
    }
    if(child == 0){                                 // child process goes through here 
        close(cmd->out_pipe[PREAD]);                // child not using the "read" pipe so we close it 
        dup2(cmd->out_pipe[PWRITE],STDOUT_FILENO); 
        execvp(cmd->name,cmd->argv); 
    }else{                                          // parent process goes through here 
        cmd->pid = child;  
        close(cmd->out_pipe[PWRITE]); 
        snprintf(cmd->str_status,STATUS_LEN,"RUN");
    }
}
// Forks a process and executes command in cmd in the process.
// Changes the str_status field to "RUN" using snprintf().  Creates a
// pipe for out_pipe to capture standard output.  In the parent
// process, ensures that the pid field is set to the child PID. In the
// child process, directs standard output to the pipe using the dup2()
// command. For both parent and child, ensures that unused file
// descriptors for the pipe are closed (write in the parent, read in
// the child).

void cmd_update_state(cmd_t *cmd, int block){
    if(cmd->finished != 0){
        int status; 
        while(1){
            waitpid(cmd->pid,&status,block);          // waits for given process 
            if( WIFEXITED(status)){                   // if process has terminated  
                cmd->finished = 1;
                cmd->status = WEXITSTATUS(status);    // set status field to status of the cmd 
                snprintf(cmd->str_status,STATUS_LEN,"EXIT(%d)",cmd->status); 
                printf("@!!! %s[%d]: %s\n",cmd->name,cmd->status,cmd->str_status);
                break;                                // break out of the looop since process has finished
            } 
        }
        cmd_fetch_output(cmd);
    } 
}
// If the finished flag is 1, does nothing. Otherwise, updates the
// state of cmd.  Uses waitpid() and the pid field of command to wait
// selectively for the given process. Passes block (one of DOBLOCK or
// NOBLOCK) to waitpid() to cause either non-blocking or blocking
// waits.  Uses the macro WIFEXITED to check the returned status for
// whether the command has exited. If so, sets the finished field to 1
// and sets the cmd->status field to the exit status of the cmd using
// the WEXITSTATUS macro. Calls cmd_fetch_output() to fill up the
// output buffer for later printing.
//
// When a command finishes (the first time), prints a status update
// message of the form
//
// @!!! ls[#17331]: EXIT(0)
//
// which includes the command name, PID, and exit status.

char *read_all(int fd, int *nread){
    char *buffer = malloc(BUFSIZE + 1);               // buffer to hold output 
    int maxSize = BUFSIZE;                            // integer used to double size of buffer if needed 
    int curPos = 0; 
    while(1){    
        int endOfFile = read(fd,buffer,BUFSIZE);       // reads in BUFSIZE bytes into buffer (0 if EOF -1 if error else number of bytes read) 
        curPos += BUFSIZE;
        if(endOfFile == 0){                           // means no more data to be read in (0 for EOF)
            *nread = curPos;                          // number of bytes read 
            buffer[maxSize] = '\0'; 
            break; 
        } 
        if(curPos == maxSize){                        // if our current position in the buffer is equal to the length of the buffer
            maxSize *= 2;                            
            char *newBuf = realloc(buffer,maxSize + 1);   // double size of buffer (plus 1 for \0 character)
            buffer = newBuf; 
        }
    }
    return buffer;
}
// Reads all input from the open file descriptor fd. Assumes
// character/text output and null-terminates the character output with
// a '\0' character allowing for printf() to print it later. Stores
// the results in a dynamically allocated buffer which may need to
// grow as more data is read.  Uses an efficient growth scheme such as
// doubling the size of the buffer when additional space is
// needed. Uses realloc() for resizing.  When no data is left in fd,
// sets the integer pointed to by nread to the number of bytes read
// and return a pointer to the allocated buffer. Ensures the return
// string is null-terminated. Does not call close() on the fd as this
// is done elsewhere.

void cmd_fetch_output(cmd_t *cmd){
    if(cmd->finished == 0){
        printf("%s[#%d]: not finished yet\n",cmd->name,cmd->status);
    }else{
        cmd->output = read_all(cmd->out_pipe[0],&cmd->output_size);
        close(cmd->out_pipe[0]); 
    }
}
// If cmd->finished is zero, prints an error message with the format
// 
// ls[#12341] not finished yet
// 
// Otherwise retrieves output from the cmd->out_pipe and fills
// cmd->output setting cmd->output_size to number of bytes in
// output. Makes use of read_all() to efficiently capture
// output. Closes the pipe associated with the command after reading
// all input.

void cmd_print_output(cmd_t *cmd){
    if(cmd->output == NULL){
        printf("%s[#%d]: output not ready\n",cmd->name,cmd->status);
    }else{
        write(STDERR_FILENO,cmd->output,cmd->output_size);
    }
}
// Prints the output of the cmd contained in the output field if it is
// non-null. Prints the error message
// 
// ls[#17251] : output not ready
//
// if output is NULL. The message includes the command name and PID.
