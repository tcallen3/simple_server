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

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "network.h"
#include "parser.h"
#include "response.h"

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
  assert(!bParsed);

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

  return 0;
}
