#include "systemcalls.h"

#include <errno.h>   //errno
#include <fcntl.h>   // open
#include <stdlib.h>  //system()
#include <string.h>  //strerror
#include <sys/wait.h>//waitpid
#include <unistd.h> /* fork, std stream*/

/**
 * @param cmd the command to execute with system()
 * @return true if the command in @param cmd was executed
 *   successfully using the system() call, false if an error occurred,
 *   either in invocation of the system() call, or if a non-zero return
 *   value was returned by the command issued in @param cmd.
*/
bool do_system(const char *cmd)
{

/*
 * TODO  add your code here
 *  Call the system() function with the command set in the cmd
 *   and return a boolean true if the system() call completed with success
 *   or false() if it returned a failure
*/

    bool return_val = false;
    if (0 != system(cmd)) {
      fprintf(stderr, "Error %s", strerror(errno));
    }
    else
      return_val = true;

    return return_val;
}

/**
* @param count -The numbers of variables passed to the function. The variables are command to execute.
*   followed by arguments to pass to the command
*   Since exec() does not perform path expansion, the command to execute needs
*   to be an absolute path.
* @param ... - A list of 1 or more arguments after the @param count argument.
*   The first is always the full path to the command to execute with execv()
*   The remaining arguments are a list of arguments to pass to the command in execv()
* @return true if the command @param ... with arguments @param arguments were executed successfully
*   using the execv() call, false if an error occurred, either in invocation of the
*   fork, waitpid, or execv() command, or if a non-zero return value was returned
*   by the command issued in @param arguments with the specified arguments.
*/

bool do_exec(int count, ...)
{
    va_list args;
    va_start(args, count);
    char * command[count+1];
    int i;
    for(i=0; i<count; i++)
    {
        command[i] = va_arg(args, char *);
    }
    command[count] = NULL;
    // this line is to avoid a compile warning before your implementation is complete
    // and may be removed
    // command[count] = command[count];

/*
 * TODO:
 *   Execute a system command by calling fork, execv(),
 *   and wait instead of system (see LSP page 161).
 *   Use the command[0] as the full path to the command to execute
 *   (first argument to execv), and use the remaining arguments
 *   as second argument to the execv() command.
 *
*/

    bool return_val = false;
    pid_t pid = fork();
    if (-1 == pid) {
      fprintf(stderr, "Error creating  the child process %s\n",
              strerror(errno));
    }
    else if (0 == pid) {
      printf("Child process <%d> with command %s\n", getpid(), command[0]);
      int ret = execv(command[0], command);

      if (-1 == ret) {
        perror("perror -execv");
        exit(EXIT_FAILURE);
      }
      printf("Never reach, running %s\n", command[0]);
    }
    else {
      int wstatus;
      do {
        if (-1 == waitpid(pid, &wstatus, 0)) {
          fprintf(stderr, "Error waitpid %s\n", strerror(errno));
        }
        else if ((WIFEXITED(wstatus)) && /*Normal termination and Exited with Success*/
                   (WEXITSTATUS(wstatus) == EXIT_SUCCESS)) {

          printf("Child process <%d> exited status %d\n", pid,
                 WEXITSTATUS(wstatus));
          return_val = true;
        } else {
          printf("Normal Termination:%d, But returned Failure exec:%d\n", WIFEXITED(wstatus), WEXITSTATUS(wstatus));
        }

      } while (!WIFEXITED(wstatus));
    }

    va_end(args);

    return return_val;
}

/**
* @param outputfile - The full path to the file to write with command output.
*   This file will be closed at completion of the function call.
* All other parameters, see do_exec above
*/
bool do_exec_redirect(const char *outputfile, int count, ...)
{
    va_list args;
    va_start(args, count);
    char * command[count+1];
    int i;
    for(i=0; i<count; i++)
    {
        command[i] = va_arg(args, char *);
    }
    command[count] = NULL;
    // this line is to avoid a compile warning before your implementation is complete
    // and may be removed
    // command[count] = command[count];


/*
 * TODO
 *   Call execv, but first using https://stackoverflow.com/a/13784315/1446624 as a refernce,
 *   redirect standard out to a file specified by outputfile.
 *   The rest of the behaviour is same as do_exec()
 *
*/

    bool return_val = false;

    pid_t pid = fork();
    if (-1 == pid) {
      fprintf(stderr, "Error creating  the child process %s\n",
              strerror(errno));
    } else if (0 == pid) {
      int fd = open(outputfile, O_WRONLY | O_CREAT, 0640);

      printf("Child process <%d>\n", getpid());

      // Duplicate the FD in order to have it on the exec
      if (-1 == dup2(fd, STDOUT_FILENO)) {
        perror("perror -dup2");
      }
      close(fd);
      if (-1 == execv(command[0], command)) {
        perror("perror -execv");
        exit(EXIT_FAILURE);
      }
      printf("Never reach, running %s\n", command[0]);
    } else {
      int wstatus;

      do {
        if (-1 == waitpid(pid, &wstatus, 0)) {
          fprintf(stderr, "Error waitpid %s\n", strerror(errno));
        }
        else if ((WIFEXITED(wstatus)) && //If exited normally and with Success
                   (WEXITSTATUS(wstatus) == EXIT_SUCCESS)) {

          printf("Child process <%d> with command %s exited status %d\n", pid, command[0],
                 WEXITSTATUS(wstatus));
          return_val = true;
        }

      } while (!WIFEXITED(wstatus));
    }

    va_end(args);

    return return_val;
}
