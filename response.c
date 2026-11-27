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

#include <sys/stat.h>

#include <errno.h>
#include <fcntl.h>
#include <magic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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

char *construct_canonical_resource_path(const char *relPath) {
  char *currPath = NULL;
  char *resolvedDir = NULL;
  bool success = false;
  int savedErrno = 0;
  size_t pathSize = 0;

  currPath = getcwd(NULL, 0);
  if (currPath == NULL) {
    return NULL;
  }
  pathSize = strlen(currPath) + 1;

  /* add path separator if neeed */
  if (relPath[0] != '/') {
    success = safe_append(currPath, "/", &pathSize);
    if (!success) {
      goto fail;
    }
  }

  /* append relative path */
  success = safe_append(currPath, relPath, &pathSize);
  if (!success) {
    goto fail;
  }

  /* canonicalize result */
  resolvedDir = realpath(currPath, resolvedDir);
  if (resolvedDir == NULL) {
    savedErrno = errno;
    goto fail;
  }

  (void)free(currPath);
  return resolvedDir;

fail:
  (void)free(currPath);
  if (savedErrno != 0) {
    errno = savedErrno;
  }
  return NULL;
}

/* Note: caller owns the memory for the returned string */
char *determine_content_type(const char *fname) {
  char *result = NULL;
  size_t slen = 0;
  size_t copySize = 0;

  magic_t cookie = magic_open(MAGIC_MIME);

  /* load default database */
  int rval = magic_load(cookie, NULL);
  if (rval != 0) {
    return NULL;
  }

  /* read file type and copy result to return */
  const char *desc = magic_file(cookie, fname);
  slen = strlen(desc) + 1;
  result = (char *)malloc(slen);
  copySize = strlcpy(result, desc, slen);
  if (copySize >= slen) {
    free(result);
    return NULL;
  }

  /* return magic resources */
  magic_close(cookie);

  return result;
}

bool construct_status_line(ResponseStatus status, char *buf, size_t size) {
  const ResponseContent *content = &responses[status];
  int rval = snprintf(buf, size, "HTTP/1.0 %d %s\r\n", content->statusCode,
                      content->statusComment);
  if (rval < 0 || (size_t)rval >= size) {
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
  if (rval < 0 || (size_t)rval >= size) {
    return false;
  }

  return true;
}

bool construct_server_header(char *buf, size_t size) {
  int rval = snprintf(buf, size, "Server: %s/%d.%d.%d\r\n", SWS_SERVER_NAME,
                      SWS_MAJOR_VERSION, SWS_MINOR_VERSION, SWS_PATCH_VERSION);
  if (rval < 0 || (size_t)rval >= size) {
    return false;
  }

  return true;
}

bool construct_last_modified_header(time_t *pTime, char *buf, size_t size) {
  char timeBuf[BUFSIZ];
  struct tm *pCalendar = NULL;
  int rval = 0;

  pCalendar = gmtime(pTime);
  if (pCalendar == NULL) {
    return false;
  }

  if (!generate_http_time(pCalendar, timeBuf, BUFSIZ)) {
    return false;
  }

  rval = snprintf(buf, size, "Last-Modified: %s\r\n", timeBuf);
  if (rval < 0 || (size_t)rval >= size) {
    return false;
  }

  return true;
}

bool construct_content_type_header(const char *contentType, char *buf,
                                   size_t size) {
  int rval = snprintf(buf, size, "Content-Type: %s\r\n", contentType);
  if (rval < 0 || (size_t)rval >= size) {
    return false;
  }

  return true;
}

bool construct_content_length_header(off_t fsize, char *buf, size_t bsize) {
  int rval = snprintf(buf, bsize, "Content-Length: %ld\r\n", fsize);
  if (rval < 0 || (size_t)rval >= bsize) {
    return false;
  }

  return true;
}

/* best-effort server error message in case something goes very wrong */
void send_fallback_error_response(int sockfd) {
  (void)write(sockfd, fallbackErrorStatus, strlen(fallbackErrorStatus));
}

void send_error_response(ResponseStatus status, int sockfd) {
  char *response = construct_error_response(status);
  if (response != NULL) {
    (void)write(sockfd, response, strlen(response));
  }

  (void)free(response);
}

/* TODO: support directly enumeration and index.html */
/* TODO: support cgi-bin execution */
/* TODO: support home directory lookup */
void construct_response(RequestDetails *rd, int sockfd) {
  char buf[BUFSIZ];
  char getFinalLine[] = "\r\n\r\n";
  char *resourcePath = NULL;
  char *contentType = NULL;
  char *response = NULL;
  struct stat resourceStat;
  time_t imsTime;
  size_t responseSize = BUFSIZ;
  size_t expectSize = 0;
  ssize_t nwritten = 0;
  ssize_t nread = 0;
  bool success = false;
  int fd = -1;

  if (rd->type == REQUEST_UNKNOWN) {
    send_error_response(STATUS_NOT_IMPLEMENTED, sockfd);
    goto cleanup;
  }

  /* TODO: sanity check that path is a proper subpath of CWD (or user dir) */
  /* construct canonical absolute path via CWD prefix and realpath(3) */
  resourcePath = construct_canonical_resource_path(rd->relPath);
  if (resourcePath == NULL) {
    if (errno == EACCES) {
      send_error_response(STATUS_FORBIDDEN, sockfd);
      goto cleanup;
    } else if (errno == ENOENT || errno == ENOTDIR || errno == EINVAL) {
      send_error_response(STATUS_NOT_FOUND, sockfd);
      goto cleanup;
    } else {
      goto fail;
    }
  }

  /* confirm requested content is an extant file we have access to */
  if (stat(resourcePath, &resourceStat) == -1) {
    if (errno == EACCES) {
      send_error_response(STATUS_FORBIDDEN, sockfd);
      goto cleanup;
    } else if (errno == EBADF || errno == EFAULT || errno == ENOENT ||
               errno == ENOTDIR) {
      send_error_response(STATUS_NOT_FOUND, sockfd);
      goto cleanup;
    } else {
      goto fail;
    }
  }

  /* TEMPORARY: until we add directory support, dir-only content is an error */
  if (!S_ISREG(resourceStat.st_mode)) {
    send_error_response(STATUS_FORBIDDEN, sockfd);
    goto cleanup;
  }

  if (access(resourcePath, R_OK) == -1) {
    send_error_response(STATUS_FORBIDDEN, sockfd);
    goto cleanup;
  }

  /* check for early exit: request is GET and If-Modified-Since applies */
  if (rd->type == REQUEST_GET && rd->imsCacheCheck) {
    imsTime = mktime(&rd->imsCacheTime);
    if (imsTime == (time_t)-1) {
      goto fail;
    }

    if (difftime(resourceStat.st_mtim.tv_sec, imsTime) < 0.0) {
      send_error_response(STATUS_NOT_MODIFIED, sockfd);
      goto cleanup;
    }
  }

  /* determine content type for the file via libmagic */
  contentType = determine_content_type(resourcePath);
  if (contentType == NULL) {
    goto fail;
  }

  /* TODO: determine content length for non-file responses */

  /* successful reply status */
  response = (char *)malloc(responseSize);
  if (response == NULL) {
    goto fail;
  }
  success = construct_status_line(STATUS_OK, response, responseSize);
  if (!success) {
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

  /* last modified */
  success =
      construct_last_modified_header(&resourceStat.st_mtim.tv_sec, buf, BUFSIZ);
  if (!success) {
    goto fail;
  }
  success = safe_append(response, buf, &responseSize);
  if (!success) {
    goto fail;
  }

  /* content type */
  success = construct_content_type_header(contentType, buf, BUFSIZ);
  if (!success) {
    goto fail;
  }
  success = safe_append(response, buf, &responseSize);
  if (!success) {
    goto fail;
  }

  /* content length */
  success = construct_content_length_header(resourceStat.st_size, buf, BUFSIZ);
  if (!success) {
    goto fail;
  }
  success = safe_append(response, buf, &responseSize);
  if (!success) {
    goto fail;
  }

  /* add empty line as separator/terminator */
  success = safe_append(response, "\r\n", &responseSize);
  if (!success) {
    goto fail;
  }

  /* stream headers */
  expectSize = strlen(response);
  nwritten = write(sockfd, response, expectSize);
  if (nwritten < 0 || (size_t)nwritten != expectSize) {
    /* Not an error we can handle via HTTP responses, bail out */
    (void)free(response);
    (void)free(resourcePath);
    (void)free(contentType);
    return;
  }

  /* TODO: add logging */

  /* if request is GET, stream out file content */
  if (rd->type == REQUEST_GET) {
    fd = open(resourcePath, O_RDONLY);
    /* error is -1 which we can't do much about other than skip to the end */
    if (fd >= 0) {
      while ((nread = read(fd, buf, BUFSIZ)) > 0) {
        nwritten = write(sockfd, buf, nread);
        if (nwritten == -1) {
          /* again, we skip errors because there's nothing we can do here */
          break;
        }
      }
      (void)write(sockfd, getFinalLine, strlen(getFinalLine));
      (void)close(fd);
    }
  }

  (void)free(response);
  (void)free(resourcePath);
  (void)free(contentType);
  return;

cleanup:
  (void)free(response);
  (void)free(resourcePath);
  (void)free(contentType);
  return;

fail:
  send_fallback_error_response(sockfd);

  (void)free(response);
  (void)free(resourcePath);
  (void)free(contentType);
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
