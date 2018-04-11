/*
 * dmesg-recent - Output dmesg lines timestamped since last invocation.
 *
 * Copyright (c) 2017 nandhp <nandhp@gmail.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
  const char *stampfile = NULL;
  char *line = NULL;
  size_t len = 0;
  ssize_t nread;
  int rc = 0;
  double target = 0.0, stamp = 0.0;
  int any_multiline = 0;

  {
    int want_help = 0, has_error = 0;
    int opt;
    while ( (opt = getopt(argc, argv, "slh")) != -1 ) {
      switch (opt) {
      case 'l':
        any_multiline = 1;
        break;
      case 's':
        any_multiline = 0;
        break;
      case 'h':
        want_help = 1;
        break;
      default:
        has_error = 1;
        break;
      }
    }
    has_error = has_error || (optind + 1 != argc);

    stampfile = has_error ? NULL : argv[optind];
    if ( has_error || !stampfile || want_help ) {
      fprintf(stderr, "Usage: dmesg | %s [-m] <stampfile>\n", argv[0]);
      return want_help ? 0 : 1;
    }
  }

  /* Load stamp file */
  {
    FILE *fh = fopen(stampfile, "r");
    int nel;
    if ( fh ) {
      nel = fscanf(fh, "%lf", &target);
      if ( nel != 1 ) {
        const char *error = "Parse error";
        if ( nel == EOF && ferror(fh) )
          error = strerror(errno);
        fprintf(stderr, "%s: Error reading stampfile: %s\n", argv[0], error);
        target = 0.0;
        rc = 1;
      }
      fclose(fh);
    }
    else if ( errno != ENOENT ) {
      fprintf(stderr, "%s: Error opening stampfile for read: %s\n", argv[0],
              strerror(errno));
      rc = 1;
    }
  }

  /* Output new messages */
  while ((nread = getline(&line, &len, stdin)) != -1) {
    /* Recent dmesg may have multi-line messages */
    if ( !isspace(line[0]) ) {
      int nel = sscanf(line, "[%lf]", &stamp);
      if ( nel < 1 && !any_multiline ) {
        if ( stamp > 0 ) {
          fprintf(stderr, "%s: dmesg parse error: '%s'\n", argv[0], line);
          rc = 2;
          break;
        }
        stamp = 0;
      }
    }
    if ( stamp > target )
      if ( fwrite(line, nread, 1, stdout) != 1 ) {
        fprintf(stderr, "%s: Error writing log: %s\n", argv[0],
                ferror(stdout) ? strerror(errno) :
                feof(stdout) ? "End of file" : "Unknown error");
        rc = 2;
        break;
      }
  }
  /* Check error code */
  if ( ferror(stdin) ) {
    rc = 2;
    fprintf(stderr, "%s: Read error: %s\n", argv[0], strerror(errno));
  }
  free(line);

  /* Update stamp file */
  if ( rc < 2 ) { /* Updating stampfile OK if stampfile read error occurred */
    char tempfile[strlen(stampfile) + 5];
    strcpy(tempfile, stampfile); /* strlen(stampfile) */
    strcat(tempfile, ".tmp");  /* 4 + null byte = 5 */
    FILE *fh = fopen(tempfile, "w");
    int nel;
    if ( fh ) {
      nel = fprintf(fh, "%lf\n", stamp);
      if ( nel < 2 ) {
        const char *error = "Output error";
        if ( nel < 0 )
          error = strerror(errno);
        fprintf(stderr, "%s: Error writing stampfile: %s\n", argv[0], error);
        rc = 1;
      }
      if ( fclose(fh) != 0 ) {
        fprintf(stderr, "%s: Error closing stampfile: %s\n", argv[0],
                strerror(errno));
        rc = 1;
      }
      else if ( rename(tempfile, stampfile) != 0 ) {
        fprintf(stderr, "%s: Error renaming stampfile: %s\n", argv[0],
                strerror(errno));
        rc = 1;
      }
    }
    else {
      fprintf(stderr, "%s: Error opening stampfile for write: %s\n", argv[0],
              strerror(errno));
      rc = 1;
    }
  }
  return rc;
}
