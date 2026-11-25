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

#include "parser.h"

/*
bool parse_http_request(const char *, RequestContent *);
bool parse_http_cache_header(const char *, struct tm *);
*/

bool parse_http_time(const char *tstr, struct tm *tm) {
  char *pret = NULL;

  /* canonical: "Sun, 06 Nov 1994 08:49:37 GMT" */
  const char *canonFormat = "%a, %d %b %Y %T GMT";

  /* alternative: "Sunday, 06-Nov-94 08:49:37 GMT" */
  const char *altCanonFormat = "%a, %d-%b-%y %T GMT";

  /* asctime: "Sun Nov  6 08:49:37 1994" */
  const char *ascTimeFormat = "%a %b %d %T %Y";

  if ((pret = strptime(tstr, canonFormat, tm)) != NULL) {
    return true;
  } else if ((pret = strptime(tstr, altCanonFormat, tm)) != NULL) {
    return true;
  } else if ((pret = strptime(tstr, ascTimeFormat, tm)) != NULL) {
    return true;
  }

  return false;
}
