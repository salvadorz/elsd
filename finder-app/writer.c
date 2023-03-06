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
 * @file writer.c
 * @author Salvador Z
 * @date 9 Sep 2022
 * @brief File for write a string in a file providing the path to the file.
 *
 * Accepts 2 runtime arguments. The first argument is a full path to a file
 * (including filename), referred to below as writefile;
 * The second argument is a text string which will be written within
 * these file, referred to below as writestr. Uses syslog as logging framework
 *
 * @see https://linux.die.net/man/3/syslog
 * @see https://linux.die.net/man/8/syslogd
 */

#include <errno.h>
#include <stdio.h>  /*streams> fopen, fputs*/
#include <stdlib.h> /*NULL (stddef)*/
#include <string.h> /*strerror*/
#include <syslog.h> /*system logs*/


#define writefile (1U) /*Arg1 path to a directory on the filesystem*/
#define writestr  (2U) /*Arg2 string which will be searched within these files*/
#define MAX_ARGC (writefile + writestr)
/*The MAX is 3 bacause the first arg is the PROCESS itself*/

int main(int argc, char **argv) {
  int return_val = EXIT_SUCCESS;

  /**
   * 2.1 Setup syslog logging with process name for your utility using the
   * LOG_USER facility.
   */
  openlog(argv[0], (LOG_ODELAY | LOG_PERROR), LOG_USER);

  /** 1.2
   * Exits with error and log error(s) if any of the arguments above were not
   * specified
   */
  if (MAX_ARGC > argc) {
    syslog(LOG_INFO, "Usage: %s [FILENAME_PATH] [STRING_TO_WRITE]\n", argv[0]);
    syslog(LOG_ERR, "Invalid Number of Arguments, provided:%d\n", argc - 1);
    return_val = EPERM;
  } else if (MAX_ARGC < argc) {
    syslog(LOG_ERR, "Argument list too long, provided:%d\n", argc - 1);
    return_val = E2BIG;
  } else if (NULL == argv[writestr]) /* No need to check argv[writefile] */
  {
    syslog(LOG_ERR, "Invalid argument(s), provided:%d\n", argc - 1);
    return_val = EINVAL;
  } else if (EXIT_SUCCESS == return_val) {
    /** 1.3
     * Creates a new file with name writefile with content writestr
     * assume the directory is created by the caller.
     */
    FILE *wrfile = fopen(argv[writefile], "wb");
    if (NULL == wrfile) {
      syslog(LOG_ERR, "Failed to open the file %s. %s", argv[writefile],
             strerror(errno));
      return_val = errno;
    } else {

      /** 2.2 LOG_DEBUG
       * Use the syslog capability to write a message “Writing <string> to
       * <file>”
       */
      fputs(argv[writestr], wrfile);
      syslog(LOG_DEBUG, "Writing %s to %s.", argv[writestr], argv[writefile]);

      if (EOF == fclose(wrfile)) {
        /** 2.3
         * Use the syslog capability to log any unexpected errors with LOG_ERR
         * level.
         */
        syslog(LOG_ERR, "Not succesful fclose. %s", strerror(errno));
        return_val = errno;
      }
    }
  }

  closelog();

  return return_val;
}