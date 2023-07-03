/****************************************************************************
 * Copyright (C) 2022 by Salvador Z                                            *
 *                                                                             *
 * This file is part of ELSD                                                   *
 *                                                                             *
 *   Permission is hereby granted, free of charge, to any person obtaining a   *
 *   copy of this software and associated documentation files (the Software)   *
 *   to deal in the Software without restriction including without limitation  *
 *   the rights to use, copy, modify, merge, publish, distribute, sublicense,  *
 *   and/or sell copies ot the Software, and to permit persons to whom the     *
 *   Software is furnished to do so, subject to the following conditions:      *
 *                                                                             *
 *   The above copyright notice and this permission notice shall be included   *
 *   in all copies or substantial portions of the Software.                    *
 *                                                                             *
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS   *
 *   OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARANTIES OF MERCHANTABILITY *
 *   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL   *
 *   THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR      *
 *   OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,     *
 *   ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE        *
 *   OR OTHER DEALINGS IN THE SOFTWARE.                                        *
 ******************************************************************************/

/**
 * @file aesdsocket.c
 * @author Salvador Z
 * @date 9 Dec 2022
 * @brief File for Native Socket Server
 *
 * Opens a stream socket bound to port 9000 returns -1 if any connection fails
 * Listens for and accepts a connection
 * Logs message to the syslog “Accepted connection from xxx” where XXXX is the
 * IP address of the connected client.
 * Receives data over the connection and appends to file
 * /var/tmp/aesdsocketdata, creates this file if it doesn’t exist.
 *
 * @see https://linux.die.net/man/3/syslog
 */

#include "aesdsocket.h"

#if SCKT_DEBUG
#define DEBUG_LOG(MSG) fprintf(stderr, MSG)
#else
#define DEBUG_LOG(MSG)
#endif


sig_atomic_t sig_exit = 0;
int fd_files[FD_MAX] = {0};

void socket_register_fd(fd_reg_t fd_reg, int fd) {
  if (FD_MAX > fd_reg) {
    fd_files[fd_reg] = fd;
  }
}

void socket_cleanup(int exit_code) {
  for (int i = FD_MAX - 1; i >= 0; --i) {
    if (0 != fd_files[i]) {
      close(fd_files[i]);
    }
  }
  remove(SOCKET_DATA);
  closelog();
  exit(exit_code);
}

/*Handler to clean up any child process*/
void signal_handler(int signal_number) {
  int errno_back = errno;

  if (TRUE == sig_exit) {
    socket_cleanup(errno_back);
  }
  syslog(LOG_INFO, "Caught signal, exiting due %s", strsignal(signal_number));
  if ((SIGINT == signal_number)) // finish gracesfully
    sig_exit = TRUE;
  else if (SIGTERM == signal_number) { // finish now...
    sig_exit = TRUE;
    shutdown(fd_files[fd_sockt], SHUT_RDWR);
    // socket_cleanup(errno_back);
  }

  errno = errno_back;
}

void sig_handler_reg() {
  struct sigaction sa = {0};

  sa.sa_handler = &signal_handler;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART;
  sigaction(SIGINT, &sa, NULL);
  sigaction(SIGTERM, &sa, NULL);
}

int socket_connected_recv_data(int client_handle) {
  int exec_ok = EXIT_SUCCESS;

  // a+ append and read. If file does not exist, create
  FILE *wrfile = fopen(SOCKET_DATA, "a+");

  if (NULL == wrfile) {
    syslog(LOG_ERR, "Failed to open the aesdsocketdata. %s", strerror(errno));
    exec_ok = errno;
  } else {

    bool end_tx    = 0;
    int bytes_used = 0;
    int bytes_recv = 0;
    char buff[BUFFER_SIZE] = {0};

    do {
      // If socket not cfg as MSG_DONTWAIT, recv will block
      bytes_recv =
          recv(client_handle, &buff[bytes_used], BUFFER_SIZE - bytes_used, 0);
      if (SOCKET_ERROR == bytes_recv)
        syslog(LOG_ERR, "Failed to receive from connection. %s", strerror(errno));
      else if (bytes_recv)
        bytes_used += bytes_recv;
      else { // if recv 0
        continue;
      }

      end_tx = (NULL != strchr(buff, '\n')) ? true : false;

      // If end of transmission or Buffer full, flush to a file
      if (end_tx || BUFFER_SIZE <= bytes_used) {
        int written = fwrite(buff, sizeof(char), bytes_used, wrfile);

        // Could be due signal
        if (bytes_used != written) {
          if (EOF == written) {
            syslog(LOG_ERR, "Write Failed!. %s", strerror(errno));
            exec_ok = errno;
          }
          close(client_handle);
          fclose(wrfile);
          socket_cleanup(exec_ok);
        }
        bytes_used = 0;
        memset(buff, 0, sizeof(buff)); // strchr used so needs to be clean
      }

    } while (!end_tx || (0 < bytes_used));
    DEBUG_LOG("[OK] File stored\n");
  }

  fclose(wrfile);

  return exec_ok;
}

int socket_connected_send_data(int client_handle) {
  int exec_ok = EXIT_SUCCESS;
  int rc = 0;

  FILE *rdfile = fopen(SOCKET_DATA, "r");

  if (NULL != rdfile) {
    DEBUG_LOG("[OK] Sending back\n");
    rc = fseek(rdfile, 0, SEEK_SET);
    if (SOCKET_ERROR == rc) {
      syslog(LOG_ERR, "Write Failed!. %s", strerror(errno));
      exec_ok = errno;

      fclose(rdfile);
      socket_cleanup(exec_ok);
    }

    char buff[BUFFER_SIZE] = {0};
    ssize_t n_read = 0;
    ssize_t n_sent = 0;
    do {
      // send the whole buffer
      size_t t_sent = 0; // total sent
      while (0 < n_read) {
        n_sent = send(client_handle, &buff[t_sent], n_read, 0);
        if (n_sent > 0) {
          n_read -= n_sent;
          t_sent += n_sent;
        }
      }

      n_read = fread(buff, sizeof(char), BUFFER_SIZE, rdfile);

    } while (0 < n_read);

    fclose(rdfile);
  }

  return exec_ok;
}

/* getaddrinfo() returns a list of address structures.
   Try each address until we successfully bind(2).
   If socket(2) (or bind(2)) fails, we (close the socket
   and) try the next address. */
int socket_binding(addrinfo_handle_t res) {
  int sckt_fd = -1;
  addrinfo_handle_t rp;

  for (rp = res; rp != NULL; rp = rp->ai_next) {
    sckt_fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
    if (sckt_fd == -1)
      continue;

    if (bind(sckt_fd, rp->ai_addr, rp->ai_addrlen) == 0)
      break; /* Success */

    close(sckt_fd);
  }

  return sckt_fd;
}

int static socket_daemonize() {
  int exit_ok = EXIT_SUCCESS;
  // Create a SID for child
  if (SOCKET_ERROR == setsid())
    exit_ok = EXIT_FAILURE;

  if ((EXIT_SUCCESS == exit_ok) && (SOCKET_ERROR == (chdir("/"))))
    exit_ok = EXIT_FAILURE;

  if (EXIT_SUCCESS == exit_ok) {
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    open("/dev/null", O_RDWR);
    dup(0);
    dup(0);
  }

  return exit_ok;
}

int socket_start(bool daemon) {
  int exec_ok = EXIT_SUCCESS;

  struct addrinfo hints;
  addrinfo_handle_t rp;
  int socket_fd, rc;

  // if no port specified then default port

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;       /* Allow IPv4 */
  hints.ai_socktype = SOCK_STREAM; /* TCP socket */
  hints.ai_flags = AI_PASSIVE;     /* For wildcard IP address */
  hints.ai_protocol = 0;           /* Any protocol */
  hints.ai_canonname = NULL;
  hints.ai_addr = NULL;
  hints.ai_next = NULL;

  // `result` will contain a list of address structures.
  rc = getaddrinfo(NULL, SOCKET_PORT, &hints, &rp);
  if (rc != EXIT_SUCCESS) {

    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rc));
    syslog(LOG_ERR, "Failed to get bind info. %s", gai_strerror(rc));
    exec_ok = rc;
    socket_cleanup(exec_ok);
  }

  socket_fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
  if (SOCKET_ERROR == socket_fd) {
    syslog(LOG_ERR, "Failed to open the socket. %s", strerror(errno));

    exec_ok = errno;
    socket_cleanup(exec_ok);
  }

  DEBUG_LOG("[OK] Socket Created\n");
  socket_register_fd(fd_sockt, socket_fd);

  // Set the socket options
  int set_socket_option = 1;
  if (SOCKET_ERROR == setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR,
                                 &set_socket_option,
                                 sizeof(set_socket_option))) {
    syslog(LOG_ERR, "Failed to open the socket. %s", strerror(errno));

    exec_ok = errno;
    socket_cleanup(exec_ok);
  }

  if (EXIT_SUCCESS != bind(socket_fd, rp->ai_addr, rp->ai_addrlen)) {
    syslog(LOG_ERR, "Failed to bind the socket. %s", strerror(errno));

    exec_ok = errno;
    socket_cleanup(exec_ok);
  }

  DEBUG_LOG("[OK] Socekt binded\n");
  freeaddrinfo(rp); // free the malloc-ed mem

  pid_t pid = 1;
  if (daemon) {
    pid = fork();

    if (SOCKET_ERROR == pid) { // if error
      syslog(LOG_ERR, "Fork failed. %s", strerror(errno));
      exec_ok = errno;
      socket_cleanup(exec_ok);
    } else if (0 != pid) { // parent
      // end normally
      printf("Child process <%d> created\n", pid);
      return exec_ok;
    } else {
      // daemon
      if (EXIT_SUCCESS != socket_daemonize())
        socket_cleanup(EXIT_FAILURE);
    }
  }

  // listen for connections
  rc = listen(socket_fd, SOCKET_MAX);
  if (SOCKET_ERROR == rc) {
    syslog(LOG_ERR, "Failed to listen for connections. %s", strerror(errno));

    exec_ok = errno;
    socket_cleanup(exec_ok);
  }

  DEBUG_LOG("[OK] Ready to listen for connections...\n");

  do {
    struct sockaddr connected_address;
    socklen_t addr_len = sizeof(connected_address);
    int connection_handle;
    DEBUG_LOG("[OK] Accepting connection...\n");
    connection_handle = accept(socket_fd, &connected_address, &addr_len);
    if (SOCKET_ERROR == connection_handle) {
      syslog(LOG_ERR, "Failed to accept the connection %s", strerror(errno));
      // Shutdown
      if (EINVAL != errno) {
        exec_ok = errno;
        close(socket_fd);
        socket_cleanup(exec_ok);
      }  
    } else {
      // Log the accepted connection
      socket_register_fd(fd_cnctn, connection_handle);
      char ip_address[INET_ADDRSTRLEN]; // for IPV4
      struct sockaddr_in *connected_addr =
          (struct sockaddr_in *)&connected_address;
      inet_ntop(AF_INET, &(connected_addr)->sin_addr, ip_address,
                INET_ADDRSTRLEN);

      syslog(LOG_INFO, "Accepted connection from %s", ip_address);
    }

    if (!sig_exit && EXIT_SUCCESS == exec_ok) {
      exec_ok = socket_connected_recv_data(connection_handle);
    }

    if (!sig_exit && EXIT_SUCCESS == exec_ok) {
      exec_ok = socket_connected_send_data(connection_handle);
    }
    close(connection_handle);

  } while (!sig_exit && (EXIT_SUCCESS == exec_ok));

  close(socket_fd);

  return exec_ok;
}

int main(int argc, char **argv) {
  int return_val = EXIT_SUCCESS;

  /**
   * Setup syslog logging with process name using the LOG_USER facility.
   */
   openlog(argv[0], (LOG_ODELAY | LOG_PERROR), LOG_USER);

  /** 1.2
   * -d argument which runs the aesdsocket application as a daemon.
   */
  if ((SCKT_MAX_ARG == argc) && (0 == strcmp("-d", argv[1]))) {

    sig_handler_reg();
    return_val = socket_start(true);

  } else if ((SCKT_NO_ARGS == argc) && (EXIT_SUCCESS == return_val)) {
    fprintf(stderr, "Starting aesdsocket as normal process...\n");

    sig_handler_reg();
    return_val = socket_start(false);
  }
  remove(SOCKET_DATA);
  fprintf(stderr, "Ending aesdsocket from main with code %d...\n", return_val);
  closelog();

  return return_val;
}