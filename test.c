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
#include <stdio.h>

#include "parser.h"

void test_parse_empty_time() {
  printf("%s...", __FUNCTION__);

  const char *time = "\0";
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

int main() {
  test_parse_empty_time();
  test_parse_gibberish_time();
  test_parse_http_canon_time();
  test_parse_http_canon_alt_time();
  test_parse_asc_time();
  test_parse_bad_format_time();

  return 0;
}
