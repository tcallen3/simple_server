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

#ifndef SERVER_RESPONSE_H
#define SERVER_RESPONSE_H

#include "network.h"

typedef enum ResponseStatus {
  STATUS_OK = 0,
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
} ResponseStatus;

typedef struct ResponseContent {
  int statusCode;
  char statusComment[256];
} ResponseContent;

static const ResponseContent responses[] = {
    {.statusCode = 200, .statusComment = "OK"},
    {.statusCode = 201, .statusComment = "Created"},
    {.statusCode = 202, .statusComment = "Accepted"},
    {.statusCode = 204, .statusComment = "No Content"},
    {.statusCode = 301, .statusComment = "Moved Permanently"},
    {.statusCode = 302, .statusComment = "Moved Temporarily"},
    {.statusCode = 304, .statusComment = "Not Modified"},
    {.statusCode = 400, .statusComment = "Bad Request"},
    {.statusCode = 401, .statusComment = "Unauthorized"},
    {.statusCode = 403, .statusComment = "Forbidden"},
    {.statusCode = 404, .statusComment = "Not Found"},
    {.statusCode = 500, .statusComment = "Internal Server Error"},
    {.statusCode = 501, .statusComment = "Not Implemented"},
    {.statusCode = 502, .statusComment = "Bad Gateway"},
    {.statusCode = 503, .statusComment = "Service Unavailable"},
};

/* hard-coded error status line in case our response construtors fail */
static const char fallbackErrorStatus[] =
    "HTTP/1.0 500 Internal Server Error\r\n\r\n";

void construct_response(RequestDetails *, int);
char *construct_error_response(ResponseStatus);

/* exposing this for testing */
bool safe_append(char *, const char *, size_t *);

#endif
