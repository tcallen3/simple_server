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

/* Note: this must precede the other headers so we pick up _XOPEN_SOURCE for
 * strptime availability */
#include "parser.h"

#include <sys/stat.h>

#include <assert.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "network.h"
#include "response.h"
#include "version.h"

static const char tempDirName[] = "temp";
static const char tempFileName[] = "testfile";
static const char testText[] = "hello from test.c";

bool test_construct_status_line(ResponseStatus status, char *buf, size_t size) {
  const ResponseContent *content = &responses[status];
  int rval = snprintf(buf, size, "HTTP/1.0 %d %s\r\n", content->statusCode,
                      content->statusComment);
  if (rval < 0 || (size_t)rval >= size) {
    return false;
  }

  return true;
}

void test_parse_empty_time() {
  printf("%s...", __FUNCTION__);

  const char *time = "";
  struct tm tstruct;

  /* parsing an empty time string should fail */
  bool bParsed = parse_http_time(time, &tstruct);
  assert(!bParsed);

  printf("PASSED\n");
}

void test_parse_gibberish_time() {
  printf("%s...", __FUNCTION__);

  const char *time = "this isn't a time!";
  struct tm tstruct;

  /* parsing an meaningless time string should fail */
  bool bParsed = parse_http_time(time, &tstruct);
  assert(!bParsed);

  printf("PASSED\n");
}

void test_parse_http_canon_time() {
  printf("%s...", __FUNCTION__);

  /* the preferred example from RFC 1945 */
  const char *time = "Sun, 06 Nov 1994 08:49:37 GMT";
  struct tm tstruct;

  /* parsing a canonical HTTP-format time string should work */
  bool bParsed = parse_http_time(time, &tstruct);
  assert(bParsed);

  /* see tm(3type) man page */
  assert(tstruct.tm_wday == 0);
  assert(tstruct.tm_mday == 6);
  assert(tstruct.tm_mon == 10);
  assert(tstruct.tm_year == 94);
  assert(tstruct.tm_hour == 8);
  assert(tstruct.tm_min == 49);
  assert(tstruct.tm_sec == 37);

  printf("PASSED\n");
}

void test_parse_http_canon_alt_time() {
  printf("%s...", __FUNCTION__);

  /* the obsolete-but-accepted example from RFC 1945 */
  const char *time = "Sunday, 06-Nov-94 08:49:37 GMT";
  struct tm tstruct;

  /* parsing a canonical HTTP-format time string should work */
  bool bParsed = parse_http_time(time, &tstruct);
  assert(bParsed);

  /* see tm(3type) man page */
  assert(tstruct.tm_wday == 0);
  assert(tstruct.tm_mday == 6);
  assert(tstruct.tm_mon == 10);
  assert(tstruct.tm_year == 94);
  assert(tstruct.tm_hour == 8);
  assert(tstruct.tm_min == 49);
  assert(tstruct.tm_sec == 37);

  printf("PASSED\n");
}

void test_parse_asc_time() {
  printf("%s...", __FUNCTION__);

  /* the asctime example from RFC 1945 */
  const char *time = "Sun Nov  6 08:49:37 1994";
  struct tm tstruct;

  /* parsing an asctime-style time string should work */
  bool bParsed = parse_http_time(time, &tstruct);
  assert(bParsed);

  /* see tm(3type) man page */
  assert(tstruct.tm_wday == 0);
  assert(tstruct.tm_mday == 6);
  assert(tstruct.tm_mon == 10);
  assert(tstruct.tm_year == 94);
  assert(tstruct.tm_hour == 8);
  assert(tstruct.tm_min == 49);
  assert(tstruct.tm_sec == 37);

  printf("PASSED\n");
}

void test_parse_bad_format_time() {
  printf("%s...", __FUNCTION__);

  /* close, but not actually a supported format */
  const char *time = "Sun, Nov 06 1994 08:49:37 GMT";
  struct tm tstruct;

  /* parsing an empty time string should fail */
  bool bParsed = parse_http_time(time, &tstruct);
  assert(!bParsed);

  printf("PASSED\n");
}

void test_parse_empty_request() {
  printf("%s...", __FUNCTION__);

  char *request = "";
  RequestDetails rd;
  init_request_details(&rd);

  bool bParsed = parse_http_request(request, &rd);
  assert(!bParsed);

  destroy_request_details(&rd);

  printf("PASSED\n");
}

void test_parse_truncated_request() {
  printf("%s...", __FUNCTION__);

  char request[1024];
  const char *reqType = "GET";
  RequestDetails rd;

  init_request_details(&rd);
  snprintf(request, sizeof(request) - 1, "%s", reqType);

  bool bParsed = parse_http_request(request, &rd);
  assert(!bParsed);

  destroy_request_details(&rd);

  printf("PASSED\n");
}

void test_parse_get_request() {
  printf("%s...", __FUNCTION__);

  char request[1024];
  const char *reqType = "GET";
  const char *reqPath = "/some/path";
  RequestDetails rd;

  init_request_details(&rd);
  snprintf(request, sizeof(request) - 1, "%s %s HTTP/1.0\r\n", reqType,
           reqPath);

  bool bParsed = parse_http_request(request, &rd);
  assert(bParsed);
  assert(rd.type == REQUEST_GET);
  assert(strcmp(reqPath, rd.relPath) == 0);

  destroy_request_details(&rd);

  printf("PASSED\n");
}

void test_parse_head_request() {
  printf("%s...", __FUNCTION__);

  char request[1024];
  const char *reqType = "HEAD";
  const char *reqPath = "/some/path";
  RequestDetails rd;

  init_request_details(&rd);
  snprintf(request, sizeof(request) - 1, "%s %s HTTP/1.0\r\n", reqType,
           reqPath);

  bool bParsed = parse_http_request(request, &rd);
  assert(bParsed);
  assert(rd.type == REQUEST_HEAD);
  assert(strcmp(reqPath, rd.relPath) == 0);

  destroy_request_details(&rd);

  printf("PASSED\n");
}

void test_parse_unsupported_request() {
  printf("%s...", __FUNCTION__);

  char request[] = "POST /some/path HTTP/1.0\r\n";
  RequestDetails rd;

  init_request_details(&rd);

  bool bParsed = parse_http_request(request, &rd);
  assert(bParsed);
  assert(rd.type == REQUEST_UNKNOWN);

  destroy_request_details(&rd);

  printf("PASSED\n");
}

/* Documenting that we don't use the protocol portion of the request */
void test_parse_request_no_protocol() {
  printf("%s...", __FUNCTION__);

  char request[1024];
  const char *reqType = "GET";
  const char *reqPath = "/some/path";
  RequestDetails rd;

  init_request_details(&rd);
  snprintf(request, sizeof(request) - 1, "%s %s", reqType, reqPath);

  bool bParsed = parse_http_request(request, &rd);
  assert(bParsed);
  assert(rd.type == REQUEST_GET);
  assert(strcmp(reqPath, rd.relPath) == 0);

  destroy_request_details(&rd);

  printf("PASSED\n");
}

/* The final version of the server should support several flavors of path, so
 * we confirm here that parsing doesn't care which one we provide - note that
 * the actual logic to handle the different flavors happens elsewhere. */
void multipath_parse_helper(const char *testPath) {
  char request[1024];
  const char *reqType = "GET";

  RequestDetails rd;
  init_request_details(&rd);

  snprintf(request, sizeof(request) - 1, "%s %s", reqType, testPath);
  bool bParsed = parse_http_request(request, &rd);
  assert(bParsed);
  assert(rd.type == REQUEST_GET);
  assert(strcmp(testPath, rd.relPath) == 0);

  destroy_request_details(&rd);
}

void test_parse_all_request_path_types() {
  printf("%s...", __FUNCTION__);

  const char *rootPath = "/";
  const char *vanillaPath = "/some/path";
  const char *cgiPath = "/cgi-bin/some/path";
  const char *homePath = "/~/some/path";

  multipath_parse_helper(rootPath);
  multipath_parse_helper(vanillaPath);
  multipath_parse_helper(cgiPath);
  multipath_parse_helper(homePath);

  printf("PASSED\n");
}

void test_parse_empty_header() {
  printf("%s...", __FUNCTION__);

  char request[] = "";
  RequestDetails rd;

  init_request_details(&rd);
  bool bParsed = parse_http_cache_header(request, &rd);
  assert(!bParsed);
  assert(!rd.imsCacheCheck);
  destroy_request_details(&rd);

  printf("PASSED\n");
}

void test_parse_incomplete_header() {
  printf("%s...", __FUNCTION__);

  char request[] = "If-Modified-Since:";
  RequestDetails rd;

  init_request_details(&rd);
  bool bParsed = parse_http_cache_header(request, &rd);
  assert(!bParsed);
  assert(!rd.imsCacheCheck);
  destroy_request_details(&rd);

  printf("PASSED\n");
}

void test_parse_unsupported_header() {
  printf("%s...", __FUNCTION__);

  char request[] = "Date: Sun, 06 Nov 1994 08:49:37 GMT\r\n";
  RequestDetails rd;

  init_request_details(&rd);
  bool bParsed = parse_http_cache_header(request, &rd);
  assert(!bParsed);
  assert(!rd.imsCacheCheck);
  destroy_request_details(&rd);

  printf("PASSED\n");
}

void test_parse_canon_time_header() {
  printf("%s...", __FUNCTION__);

  char request[] = "If-Modified-Since: Sun, 06 Nov 1994 08:49:37 GMT\r\n";
  RequestDetails rd;

  init_request_details(&rd);
  bool bParsed = parse_http_cache_header(request, &rd);
  assert(bParsed);
  assert(rd.imsCacheCheck);

  /* see tm(3type) man page */
  assert(rd.imsCacheTime.tm_wday == 0);
  assert(rd.imsCacheTime.tm_mday == 6);
  assert(rd.imsCacheTime.tm_mon == 10);
  assert(rd.imsCacheTime.tm_year == 94);
  assert(rd.imsCacheTime.tm_hour == 8);
  assert(rd.imsCacheTime.tm_min == 49);
  assert(rd.imsCacheTime.tm_sec == 37);
  destroy_request_details(&rd);

  printf("PASSED\n");
}

void test_safe_append_no_excess() {
  printf("%s...", __FUNCTION__);

  char first[] = "first";
  char second[] = " second";
  char expected[] = "first second";
  size_t initialLen = strlen(first) + 1;
  int result = 0;
  bool success = false;

  char *dst = (char *)malloc(initialLen);
  if (dst == NULL) {
    assert(false);
  }

  result = snprintf(dst, initialLen, "%s", first);
  if (result < 0 || result >= (int)initialLen) {
    (void)free(dst);
    assert(false);
  }

  success = safe_append(dst, second, &initialLen);
  assert(success);
  assert(strcmp(dst, expected) == 0);
  assert(initialLen == strlen(expected) + 1);

  (void)free(dst);
  printf("PASSED\n");
}

void test_safe_append_off_by_one() {
  printf("%s...", __FUNCTION__);

  char first[] = "first";
  char second[] = " second";
  char expected[] = "first second";
  size_t initialLen = strlen(first) + strlen(second);
  size_t savedLen = initialLen;
  int result = 0;
  bool success = false;

  char *dst = (char *)malloc(initialLen);
  if (dst == NULL) {
    assert(false);
  }

  result = snprintf(dst, initialLen, "%s", first);
  if (result < 0 || result >= (int)initialLen) {
    (void)free(dst);
    assert(false);
  }

  success = safe_append(dst, second, &initialLen);
  assert(success);
  assert(strcmp(dst, expected) == 0);
  assert(initialLen == strlen(expected) + 1);
  assert(initialLen > savedLen);

  (void)free(dst);
  printf("PASSED\n");
}

void test_safe_append_exact() {
  printf("%s...", __FUNCTION__);

  char first[] = "first";
  char second[] = " second";
  char expected[] = "first second";
  size_t initialLen = strlen(first) + strlen(second) + 1;
  size_t savedLen = initialLen;
  int result = 0;
  bool success = false;

  char *dst = (char *)malloc(initialLen);
  if (dst == NULL) {
    assert(false);
  }

  result = snprintf(dst, initialLen, "%s", first);
  if (result < 0 || result >= (int)initialLen) {
    (void)free(dst);
    assert(false);
  }

  success = safe_append(dst, second, &initialLen);
  assert(success);
  assert(strcmp(dst, expected) == 0);
  assert(initialLen == strlen(expected) + 1);
  assert(initialLen == savedLen);

  (void)free(dst);
  printf("PASSED\n");
}

void test_safe_append_excess() {
  printf("%s...", __FUNCTION__);

  char first[] = "first";
  char second[] = " second";
  char expected[] = "first second";
  size_t initialLen = 1024;
  size_t savedLen = initialLen;
  int result = 0;
  bool success = false;

  char *dst = (char *)malloc(initialLen);
  if (dst == NULL) {
    assert(false);
  }

  result = snprintf(dst, initialLen, "%s", first);
  if (result < 0 || result >= (int)initialLen) {
    (void)free(dst);
    assert(false);
  }

  success = safe_append(dst, second, &initialLen);
  assert(success);
  assert(strcmp(dst, expected) == 0);
  assert(initialLen == savedLen);

  (void)free(dst);
  printf("PASSED\n");
}

void test_error_response_generation() {
  printf("%s...", __FUNCTION__);

  const char msgSep[] = "\r\n";
  const size_t codeCount = 15;
  ResponseStatus codes[] = {
      STATUS_OK,
      STATUS_CREATED,
      STATUS_ACCEPTED,
      STATUS_NO_CONTENT,
      STATUS_MOVED_PERMANENTLY,
      STATUS_MOVED_TEMPORARILY,
      STATUS_NOT_MODIFIED,
      STATUS_BAD_REQUEST,
      STATUS_UNAUTHORIZED,
      STATUS_FORBIDDEN,
      STATUS_NOT_FOUND,
      STATUS_SERVER_ERROR,
      STATUS_NOT_IMPLEMENTED,
      STATUS_BAD_GATEWAY,
      STATUS_SERVICE_UNAVAILABLE,
  };
  const char datePrefix[] = "Date: ";
  const char canonFormat[] = "%a, %d %b %Y %T GMT";

  char expectedBuf[BUFSIZ];
  char *response = NULL;
  char *msgComponent = NULL;
  char *tmp = NULL;
  struct tm dummyTime;

  /* Note: we enforce specific header ordering here,
   * even though HTTP doesn't */
  for (size_t i = 0; i < codeCount; i++) {
    response = construct_error_response(codes[i]);
    assert(response != NULL);

    /* status line is what we expect */
    msgComponent = strtok(response, msgSep);
    assert(msgComponent != NULL);

    snprintf(expectedBuf, BUFSIZ, "HTTP/1.0 %d %s",
             responses[codes[i]].statusCode, responses[codes[i]].statusComment);
    assert(strcmp(msgComponent, expectedBuf) == 0);

    /* date header starts with "Date: " */
    msgComponent = strtok(NULL, msgSep);
    assert(msgComponent != NULL);
    assert(strncmp(datePrefix, msgComponent, strlen(datePrefix)) == 0);

    /* date value can be parsed via canonical strptime */
    tmp = msgComponent + strlen(datePrefix);
    assert(strptime(tmp, canonFormat, &dummyTime) != NULL);

    /* server info header */
    msgComponent = strtok(NULL, msgSep);
    assert(msgComponent != NULL);

    snprintf(expectedBuf, BUFSIZ, "Server: %s/%d.%d.%d", SWS_SERVER_NAME,
             SWS_MAJOR_VERSION, SWS_MINOR_VERSION, SWS_PATCH_VERSION);
    assert(strcmp(msgComponent, expectedBuf) == 0);

    /* no further headers */
    msgComponent = strtok(NULL, msgSep);
    assert(msgComponent == NULL);

    (void)free(response);
  }

  printf("PASSED\n");
}

void helper_compare_responses(RequestDetails *rd, ResponseStatus status) {
  int sv[2];
  int rval = 0;
  ssize_t nread = 0;
  char receivedBuf[BUFSIZ];
  char expectedBuf[BUFSIZ];
  bool success = false;

  rval = socketpair(AF_UNIX, SOCK_STREAM, 0, sv);
  assert(rval == 0);

  construct_response(rd, sv[0]);
  nread = read(sv[1], receivedBuf, BUFSIZ);
  if (nread < 0 || nread >= BUFSIZ) {
    assert(false);
  }

  success = test_construct_status_line(status, expectedBuf, BUFSIZ);
  assert(success);

  assert(strncmp(receivedBuf, expectedBuf, strlen(expectedBuf)) == 0);

  (void)close(sv[0]);
  (void)close(sv[1]);
}

int create_temp_text_file(char *fname, size_t size) {
  ssize_t nwritten = 0;
  ssize_t expectedBytes = 0;
  int fd = -1;

  if (mkdir(tempDirName, S_IRWXU) == -1) {
    return -1;
  }
  snprintf(fname, size, "%s/%s", tempDirName, tempFileName);
  fd = open(fname, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);

  expectedBytes = strlen(testText) + 1;
  nwritten = write(fd, testText, expectedBytes);
  if (nwritten < 0 || nwritten != expectedBytes) {
    assert(false);
  }

  return fd;
}

void remove_temp_text_file(int fd, char *fname) {
  (void)close(fd);
  (void)unlink(fname);
  (void)rmdir(tempDirName);
}

void test_unsupported_method_response() {
  printf("%s...", __FUNCTION__);

  RequestDetails rd;

  init_request_details(&rd);
  rd.type = REQUEST_UNKNOWN;

  helper_compare_responses(&rd, STATUS_NOT_IMPLEMENTED);

  destroy_request_details(&rd);

  printf("PASSED\n");
}

void test_nonexistent_resource_response() {
  printf("%s...", __FUNCTION__);

  RequestDetails rd;

  init_request_details(&rd);
  rd.type = REQUEST_HEAD;
  rd.relPath = (char *)malloc(BUFSIZ);
  snprintf(rd.relPath, BUFSIZ, "bad/path");

  helper_compare_responses(&rd, STATUS_NOT_FOUND);

  destroy_request_details(&rd);

  printf("PASSED\n");
}

void test_head_file_response() {
  printf("%s...", __FUNCTION__);

  const char tokSep[] = "\r\n";
  const char datePrefix[] = "Date: ";
  const char modifiedPrefix[] = "Last-Modified: ";
  const char canonFormat[] = "%a, %d %b %Y %T GMT";
  const char contentPrefix[] = "Content-Type: ";
  const char lengthPrefix[] = "Content-Length: ";

  char *tmp = NULL;
  struct tm dummyTime;
  char receivedBuf[BUFSIZ];
  char expectedBuf[BUFSIZ];
  char *tok = NULL;
  int sv[2];
  int fd, rval;
  ssize_t nread;
  RequestDetails rd;

  init_request_details(&rd);
  rd.type = REQUEST_HEAD;
  rd.relPath = (char *)malloc(BUFSIZ);
  assert(rd.relPath != NULL);

  fd = create_temp_text_file(rd.relPath, BUFSIZ);
  assert(fd != -1);

  rval = socketpair(AF_UNIX, SOCK_STREAM, 0, sv);
  assert(rval == 0);

  construct_response(&rd, sv[0]);
  nread = read(sv[1], receivedBuf, BUFSIZ);
  if (nread < 0 || nread >= BUFSIZ) {
    assert(false);
  }

  /* check status line */
  tok = strtok(receivedBuf, tokSep);
  assert(tok != NULL);

  snprintf(expectedBuf, BUFSIZ, "HTTP/1.0 %d %s",
           responses[STATUS_OK].statusCode, responses[STATUS_OK].statusComment);
  assert(strcmp(tok, expectedBuf) == 0);

  /* check date */
  tok = strtok(NULL, tokSep);
  assert(tok != NULL);
  assert(strncmp(datePrefix, tok, strlen(datePrefix)) == 0);

  /* date value can be parsed via canonical strptime */
  tmp = tok + strlen(datePrefix);
  assert(strptime(tmp, canonFormat, &dummyTime) != NULL);

  /* check server */
  tok = strtok(NULL, tokSep);
  assert(tok != NULL);

  snprintf(expectedBuf, BUFSIZ, "Server: %s/%d.%d.%d", SWS_SERVER_NAME,
           SWS_MAJOR_VERSION, SWS_MINOR_VERSION, SWS_PATCH_VERSION);
  assert(strcmp(tok, expectedBuf) == 0);

  /* check last-modified */
  tok = strtok(NULL, tokSep);
  assert(tok != NULL);
  assert(strncmp(modifiedPrefix, tok, strlen(modifiedPrefix)) == 0);

  /* date value can be parsed via canonical strptime */
  tmp = tok + strlen(modifiedPrefix);
  assert(strptime(tmp, canonFormat, &dummyTime) != NULL);

  /* check content-type prefix */
  tok = strtok(NULL, tokSep);
  assert(tok != NULL);
  assert(strncmp(contentPrefix, tok, strlen(contentPrefix)) == 0);

  /* check content-length, specific type is tricky, so we omit that portion */
  tok = strtok(NULL, tokSep);
  assert(tok != NULL);
  assert(strncmp(lengthPrefix, tok, strlen(lengthPrefix)) == 0);

  tmp = tok + strlen(modifiedPrefix);
  assert(atoi(tmp) == strlen(testText) + 1);

  /* check end of stream */
  tok = strtok(NULL, tokSep);
  assert(tok == NULL);

  (void)close(sv[0]);
  (void)close(sv[1]);
  remove_temp_text_file(fd, rd.relPath);

  destroy_request_details(&rd);

  printf("PASSED\n");
}

void test_head_ims_header_response() {
  printf("%s...", __FUNCTION__);

  int fd;
  time_t currTime;
  RequestDetails rd;

  init_request_details(&rd);

  rd.type = REQUEST_HEAD;
  rd.relPath = (char *)malloc(BUFSIZ);
  assert(rd.relPath != NULL);

  fd = create_temp_text_file(rd.relPath, BUFSIZ);
  assert(fd != -1);

  rd.imsCacheCheck = true;

  currTime = time(NULL);
  (void)gmtime_r(&currTime, &rd.imsCacheTime);

  /* set year ahead so IMS would return Not Modified */
  rd.imsCacheTime.tm_year++;

  /* If-Modified-Since not supported for HEAD, so we should get standard
   * response */
  helper_compare_responses(&rd, STATUS_OK);

  remove_temp_text_file(fd, rd.relPath);

  destroy_request_details(&rd);

  printf("PASSED\n");
}

void test_head_dir_with_index_response() {
  printf("%s...", __FUNCTION__);

  /* returns HEAD for index.html */

  printf("NOT IMPLEMENTED\n");
}

void test_head_dir_no_index_response() {
  printf("%s...", __FUNCTION__);

  /* builds and returns HEAD for directory index */

  printf("NOT IMPLEMENTED\n");
}

void test_head_user_dir_response() {
  printf("%s...", __FUNCTION__);

  printf("NOT IMPLEMENTED\n");
}

void test_get_file_response() {
  printf("%s...", __FUNCTION__);

  const char tokSep[] = "\r\n";
  const char datePrefix[] = "Date: ";
  const char modifiedPrefix[] = "Last-Modified: ";
  const char canonFormat[] = "%a, %d %b %Y %T GMT";
  const char contentPrefix[] = "Content-Type: ";
  const char lengthPrefix[] = "Content-Length: ";

  char *tmp = NULL;
  struct tm dummyTime;
  char receivedBuf[BUFSIZ];
  char expectedBuf[BUFSIZ];
  char *tok = NULL;
  int sv[2];
  int fd, rval;
  ssize_t nread;
  RequestDetails rd;

  init_request_details(&rd);
  rd.type = REQUEST_GET;
  rd.relPath = (char *)malloc(BUFSIZ);
  assert(rd.relPath != NULL);

  fd = create_temp_text_file(rd.relPath, BUFSIZ);
  assert(fd != -1);

  rval = socketpair(AF_UNIX, SOCK_STREAM, 0, sv);
  assert(rval == 0);

  construct_response(&rd, sv[0]);
  nread = read(sv[1], receivedBuf, BUFSIZ);
  if (nread < 0 || nread >= BUFSIZ) {
    assert(false);
  }

  /* check status line */
  tok = strtok(receivedBuf, tokSep);
  assert(tok != NULL);

  snprintf(expectedBuf, BUFSIZ, "HTTP/1.0 %d %s",
           responses[STATUS_OK].statusCode, responses[STATUS_OK].statusComment);
  assert(strcmp(tok, expectedBuf) == 0);

  /* check date */
  tok = strtok(NULL, tokSep);
  assert(tok != NULL);
  assert(strncmp(datePrefix, tok, strlen(datePrefix)) == 0);

  /* date value can be parsed via canonical strptime */
  tmp = tok + strlen(datePrefix);
  assert(strptime(tmp, canonFormat, &dummyTime) != NULL);

  /* check server */
  tok = strtok(NULL, tokSep);
  assert(tok != NULL);

  snprintf(expectedBuf, BUFSIZ, "Server: %s/%d.%d.%d", SWS_SERVER_NAME,
           SWS_MAJOR_VERSION, SWS_MINOR_VERSION, SWS_PATCH_VERSION);
  assert(strcmp(tok, expectedBuf) == 0);

  /* check last-modified */
  tok = strtok(NULL, tokSep);
  assert(tok != NULL);
  assert(strncmp(modifiedPrefix, tok, strlen(modifiedPrefix)) == 0);

  /* date value can be parsed via canonical strptime */
  tmp = tok + strlen(modifiedPrefix);
  assert(strptime(tmp, canonFormat, &dummyTime) != NULL);

  /* check content-type prefix, specific type is tricky, so omit that portion */
  tok = strtok(NULL, tokSep);
  assert(tok != NULL);
  assert(strncmp(contentPrefix, tok, strlen(contentPrefix)) == 0);

  /* check content-length */
  tok = strtok(NULL, tokSep);
  assert(tok != NULL);
  assert(strncmp(lengthPrefix, tok, strlen(lengthPrefix)) == 0);

  tmp = tok + strlen(modifiedPrefix);
  assert(atoi(tmp) == strlen(testText) + 1);

  /* check actual file content */
  tok = strtok(NULL, tokSep);
  assert(tok != NULL);
  assert(strncmp(testText, tok, strlen(testText)) == 0);

  /* check end of stream */
  tok = strtok(NULL, tokSep);
  assert(tok == NULL);

  (void)close(sv[0]);
  (void)close(sv[1]);
  remove_temp_text_file(fd, rd.relPath);

  destroy_request_details(&rd);

  printf("PASSED\n");
}

void test_get_ims_no_mod_response() {
  printf("%s...", __FUNCTION__);

  int fd;
  time_t currTime;
  RequestDetails rd;

  init_request_details(&rd);

  rd.type = REQUEST_GET;
  rd.relPath = (char *)malloc(BUFSIZ);
  assert(rd.relPath != NULL);

  fd = create_temp_text_file(rd.relPath, BUFSIZ);
  assert(fd != -1);

  rd.imsCacheCheck = true;

  currTime = time(NULL);
  (void)gmtime_r(&currTime, &rd.imsCacheTime);

  /* set year ahead so IMS would return Not Modified */
  rd.imsCacheTime.tm_year++;

  /* should return 304 Not Modified for GET request */
  helper_compare_responses(&rd, STATUS_NOT_MODIFIED);

  remove_temp_text_file(fd, rd.relPath);

  destroy_request_details(&rd);

  printf("PASSED\n");
}

void test_get_ims_modified_response() {
  printf("%s...", __FUNCTION__);

  int fd;
  time_t currTime;
  RequestDetails rd;

  init_request_details(&rd);

  rd.type = REQUEST_GET;
  rd.relPath = (char *)malloc(BUFSIZ);
  assert(rd.relPath != NULL);

  fd = create_temp_text_file(rd.relPath, BUFSIZ);
  assert(fd != -1);

  rd.imsCacheCheck = true;

  currTime = time(NULL);
  (void)gmtime_r(&currTime, &rd.imsCacheTime);

  /* set year behind so IMS check shows modifications */
  rd.imsCacheTime.tm_year--;

  /* should get a regular successful GET response */
  helper_compare_responses(&rd, STATUS_OK);

  remove_temp_text_file(fd, rd.relPath);

  destroy_request_details(&rd);

  printf("PASSED\n");
}

void test_get_dir_with_index_response() {
  printf("%s...", __FUNCTION__);

  /* returns contents of index.html */

  printf("NOT IMPLEMENTED\n");
}

void test_get_dir_no_index_response() {
  printf("%s...", __FUNCTION__);

  /* builds and returns directory index */

  printf("NOT IMPLEMENTED\n");
}

void test_get_user_dir_response() {
  printf("%s...", __FUNCTION__);

  printf("NOT IMPLEMENTED\n");
}

int main() {
  /* time parsing */
  test_parse_empty_time();
  test_parse_gibberish_time();
  test_parse_http_canon_time();
  test_parse_http_canon_alt_time();
  test_parse_asc_time();
  test_parse_bad_format_time();

  /* primary request parsing */
  test_parse_empty_request();
  test_parse_truncated_request();
  test_parse_get_request();
  test_parse_head_request();
  test_parse_unsupported_request();
  test_parse_request_no_protocol();
  test_parse_all_request_path_types();

  /* cache header parsing */
  test_parse_empty_header();
  test_parse_incomplete_header();
  test_parse_unsupported_header();
  test_parse_canon_time_header();

  /* safe appending */
  test_safe_append_no_excess();
  test_safe_append_off_by_one();
  test_safe_append_exact();
  test_safe_append_excess();

  /* error-like responses */
  test_error_response_generation();

  /* oddball responses */
  test_unsupported_method_response();
  test_nonexistent_resource_response();

  /* HEAD responses */
  test_head_file_response();
  test_head_ims_header_response();
  test_head_dir_with_index_response();
  test_head_dir_no_index_response();
  test_head_user_dir_response();

  /* GET responses */
  test_get_file_response();
  test_get_ims_no_mod_response();
  test_get_ims_modified_response();
  test_get_dir_with_index_response();
  test_get_dir_no_index_response();
  test_get_user_dir_response();

  return 0;
}
