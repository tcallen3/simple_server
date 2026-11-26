/*

BSD 3-Clause License

Copyright (c) 2023, Thomas Allen

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parser.h"
#include "response.h"
#include "version.h"

bool safe_append(char *dst, const char *src, size_t *dstSize) {
  size_t required = strlen(dst) + strlen(src) + 1;
  size_t result = 0;

  if (required > *dstSize) {
    dst = realloc(dst, required);
    if (dst == NULL) {
      return false;
    }
    *dstSize = required;
  }

  result = strlcat(dst, src, *dstSize);
  if (result >= *dstSize) {
    /* something went wrong with our math? */
    return false;
  }

  return true;
}

bool construct_status_line(ResponseStatus status, char *buf, size_t size) {
  const ResponseContent *content = &responses[status];
  int rval = snprintf(buf, size, "HTTP/1.0 %d %s\r\n", content->statusCode,
                      content->statusComment);
  if (rval < 0 || rval >= (int)size) {
    return false;
  }

  return true;
}

bool construct_date_header(char *buf, size_t size) {
  char timeBuf[BUFSIZ];
  int rval = 0;

  if (!generate_current_http_time(timeBuf, BUFSIZ)) {
    return false;
  }

  rval = snprintf(buf, size, "Date: %s\r\n", timeBuf);
  if (rval < 0 || rval >= (int)size) {
    return false;
  }

  return true;
}

bool construct_server_header(char *buf, size_t size) {
  int rval = snprintf(buf, size, "Server: %s/%d.%d.%d\r\n", SWS_SERVER_NAME,
                      SWS_MAJOR_VERSION, SWS_MINOR_VERSION, SWS_PATCH_VERSION);
  if (rval < 0 || rval >= (int)size) {
    return false;
  }

  return true;
}

/* FIXME: need to get a handle to the socket so we can stream content out */
/* TODO: support directly enumeration and index.html */
/* TODO: support cgi-bin execution */
/* TODO: support home directory lookup */
char *construct_response(RequestDetails *rd) {
  char buf[BUFSIZ];
  char *response = NULL;
  size_t responseSize = BUFSIZ;
  bool success = false;

  if (rd->type == REQUEST_UNKNOWN) {
    response = construct_error_response(STATUS_NOT_IMPLEMENTED);
    return response;
  }

  /* construct canonical absolute path via CWD prefix and realpath(3) */

  /* TODO: sanity check that path is a proper subpath of CWD (or user dir) */

  /* confirm requested content is an extant file we have access to */

  /* TEMPORARY: until we add directory support, dir-only content is an error */

  /* check for early exit: request is GET and If-Modified-Since applies */
  /* If-Modified-Since returns 304 Not Modified if the file is unchanged */

  /* when IMS doesn't apply, we get the last-modified timestamp on the file */

  /* determine content type for the file via libmagic */

  /* open and read file to determine content-length - data is streamed, not
   * buffered, to avoid memory growth and subtle resizing concerns */

  /* construct full status line and headers for response */

  /* ... */

  /* response date */
  success = construct_date_header(buf, BUFSIZ);
  if (!success) {
    goto fail;
  }
  success = safe_append(response, buf, &responseSize);
  if (!success) {
    goto fail;
  }

  /* server info */
  success = construct_server_header(buf, BUFSIZ);
  if (!success) {
    goto fail;
  }
  success = safe_append(response, buf, &responseSize);
  if (!success) {
    goto fail;
  }

  /* ... */

  /* if request is GET, stream out file content */

  return response;

  /* FIXME: should stream out fallback error status */
fail:
  (void)free(response);
  return NULL;
}

/* Generate the status line and meaningful supported headers for a response
 * which encountered an error. If an error occurs in generating the
 * response, this function will return NULL. Note: the caller owns the
 * memory for the returned string. */
char *construct_error_response(ResponseStatus status) {
  char buf[BUFSIZ];
  char *response = NULL;
  size_t responseSize = BUFSIZ;
  size_t catSize = 0;
  bool success;

  response = (char *)malloc(BUFSIZ);
  if (response == NULL) {
    /* OOM */
    return NULL;
  }

  /* overall status */
  success = construct_status_line(status, buf, BUFSIZ);
  if (!success) {
    goto fail;
  }
  catSize = strlcpy(response, buf, BUFSIZ);
  if (catSize >= responseSize) {
    goto fail;
  }

  /* response date */
  success = construct_date_header(buf, BUFSIZ);
  if (!success) {
    goto fail;
  }
  success = safe_append(response, buf, &responseSize);
  if (!success) {
    goto fail;
  }

  /* server info */
  success = construct_server_header(buf, BUFSIZ);
  if (!success) {
    goto fail;
  }
  success = safe_append(response, buf, &responseSize);
  if (!success) {
    goto fail;
  }

  /* append final blank line */
  success = safe_append(response, "\r\n", &responseSize);
  if (!success) {
    goto fail;
  }

  return response;

fail:
  (void)free(response);
  return NULL;
}
