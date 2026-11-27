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

#include <errno.h>
#include <stdlib.h>
#include <string.h>

/* canonical: "Sun, 06 Nov 1994 08:49:37 GMT" */
static const char canonFormat[] = "%a, %d %b %Y %T GMT";

/* TODO: consider error handling debug logging? */

/* copy the resource path component so RequestDetails owns its memory */
bool copy_path_token(char *pathToken, RequestDetails *rd) {
  size_t slen, nlen;

  slen = strlen(pathToken);
  nlen = slen + 1;
  rd->relPath = (char *)malloc(nlen);
  if (rd->relPath == NULL) {
    return false;
  }
  if (strlcpy(rd->relPath, pathToken, nlen) != slen) {
    errno = ENOBUFS;
    return false;
  }

  return true;
}

/* Parse requests of the form "<request_type> <resource> <protocol>\r\n".
 * We only support GET and HEAD for this server, and we ignore the protocol
 * portion of the header. Effectively, anything beyond HTTP/1.0 gets silently
 * downgraded. */
bool parse_http_request(char *request, RequestDetails *rcontent) {
  const char *lws = " \t";
  const char *keyGet = "GET";
  const char *keyHead = "HEAD";
  char *reqToken = NULL;
  char *pathToken = NULL;

  reqToken = strtok(request, lws);
  if (reqToken == NULL) {
    /* not enough tokens */
    return false;
  }

  if (strcmp(reqToken, keyGet) == 0) {
    rcontent->type = REQUEST_GET;

    pathToken = strtok(NULL, lws);
    if (pathToken == NULL) {
      return false;
    }

    return copy_path_token(pathToken, rcontent);
  } else if (strcmp(reqToken, keyHead) == 0) {
    rcontent->type = REQUEST_HEAD;

    pathToken = strtok(NULL, lws);
    if (pathToken == NULL) {
      return false;
    }

    return copy_path_token(pathToken, rcontent);
  }

  /* no matching supported request types, but propagate for BAD_REQUEST handling
   * in response code */
  rcontent->type = REQUEST_UNKNOWN;
  pathToken = strtok(NULL, lws);
  if (pathToken == NULL) {
    return false;
  }

  return copy_path_token(pathToken, rcontent);
}

/* We only support the If-Modified-Since cache control header, which is why we
 * explicitly accept a time structure here. */
bool parse_http_cache_header(char *hstr, RequestDetails *rd) {
  const char imsHeader[] = "If-Modified-Since";

  const char *headerSep = ":";
  const char *vspace = "\r\n";
  char *headerName = NULL;
  char *headerDate = NULL;

  headerName = strtok(hstr, headerSep);
  if (headerName == NULL) {
    return false;
  }

  if (strcmp(headerName, imsHeader) == 0) {
    headerDate = strtok(NULL, vspace);
    while (headerDate != NULL && (*headerDate == ' ' || *headerDate == '\t')) {
      headerDate++;
    }
    if (headerDate == NULL) {
      return false;
    }

    if (parse_http_time(headerDate, &rd->imsCacheTime)) {
      rd->imsCacheCheck = true;
      return true;
    }
  }

  return false;
}

/* Parse any of the three supported time formats recorded in RFC 1945. We will
 * follow the guidance there of only generating the primary canonical format,
 * but we can understand any of the allowed formats. */
bool parse_http_time(const char *tstr, struct tm *tm) {
  char *pret = NULL;

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

bool generate_current_http_time(char *buf, size_t size) {
  time_t currTime = time(NULL);
  struct tm *pCalTime = gmtime(&currTime);

  return generate_http_time(pCalTime, buf, size);
}

bool generate_http_time(struct tm *pTime, char *buf, size_t size) {
  if (strftime(buf, size, canonFormat, pTime) == 0) {
    /* bad write */
    return false;
  }

  return true;
}
