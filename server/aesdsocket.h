#include <arpa/inet.h> /*inet_ntop*/
#include <errno.h>
#include <fcntl.h>      /*open*/
#include <libgen.h>     /*basename*/
#include <netdb.h>      /*addrinfo*/
#include <pthread.h>    /*pthread_mutex_t */
#include <signal.h>     /*sigaction, sigemptyset*/
#include <stdbool.h>    /*bool*/
#include <stdio.h>      /*streams> fopen, fputs, fseek*/
#include <stdlib.h>     /*NULL (stddef)*/
#include <string.h>     /*strerror*/
#include <sys/socket.h> /*socket, setsockopt*/
#include <sys/types.h>
#include <sys/wait.h> /* waitpid */
#include <syslog.h>   /* system logs*/
#include <time.h>
#include <unistd.h>   /* close, fork*/

#define USE_KDEVICE (true)
#define SOCKET_KDEV ("/dev/aesdchar")
#define SOCKET_FILE ("/var/tmp/aesdsocketdata")
#define SOCKET_PORT ("9000") /*PORT NUMBER TO LISTEN*/
#define BUFFER_SIZE (1024U)  /*BUFFER SIZE to handle stream*/
#define SOCKET_MAX  (10U)     /*Socket MAX connections*/
#define SOCKET_ERROR (-1)
#define SCKT_MAX_ARG (2U)
#define SCKT_NO_ARGS (1U)
#define TRUE         (1)

#if (USE_KDEVICE)
#define SOCKET_DATA SOCKET_KDEV
#else
#define SOCKET_DATA SOCKET_FILE
#endif

typedef struct addrinfo *addrinfo_handle_t;

typedef enum fd_reg_e { fd_sockt = 0, fd_cnctn, FD_MAX } fd_reg_t;

#define SLIST_FOREACH_SAFE(var, head, field, tvar)                             \
  for ((var) = SLIST_FIRST((head));                                            \
       (var) && ((tvar) = SLIST_NEXT((var), field), 1); (var) = (tvar))

typedef struct sckt_file_s {
  FILE *file;
  pthread_mutex_t mutex;
} sckt_file_t;

/**
 * @brief Registering a file descriptor succesfully received
 * 
 * @param fd_reg type of file_descriptor
 * @param fd file descriptor received from the systemcall
 */
void socket_register_fd(fd_reg_t fd_reg, int fd);

/**
 * @brief Function to perform the cleanup of the process by closing files
 * and return a proper code
 * 
 * @param exit_code to be returned at exit time
 */
void socket_cleanup(int exit_code);

/**
 * @brief signal handler registration set up
 */
void sig_handler_reg();

/**
 * @brief Receive data from client and store to SOCKED_DATA file
 *
 * @param client_handle fd from accepted connection
 * @return EXIT_SUCCESS if everything ok, Error otherwise
 */
int socket_connected_recv_data(int client_handle);

/**
 * @brief Send data to client from a SOCKED_DATA file
 *
 * @param client_handle fd from accepted connection
 * @return EXIT_SUCCESS if everything ok, Error otherwise
 */
int socket_connected_send_data(int client_handle);

/**
 * @brief Starts a socket conenction as a server
 *
 * @param daemon if needs to be fork'ed and create as daemon
 * @return EXIT_SUCCESS if everything ok, Error otherwise
 */
int socket_start(bool daemon);