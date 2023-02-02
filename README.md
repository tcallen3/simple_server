# NAME

`sws` - a simple web server

# SYNOPSIS

`sws [-dh] [-c dir] [-i address] [-l file] [-p port] dir`

# DESCRIPTION

`sws` is a very simple web server, which nonetheless performs many standard operations, such as binding to the given port on the given address and listening for incoming HTTP/1.0 requests. It serves these requests relative to the supplied directory (the document root). 

Although it only supports GET and HEAD requests, along with the If-Modified-Since Request-Header (all as specified in RFC1945), `sws` does include some interesting features. In particular, it suports cgi-bin execution as laid out in RFC3875, and it provides automatic directory indexing for any directories that do not already contain an `index.html` file. It will also automatically translate requests beginning with "~" to the user's personal `sws` directory (i.e. `/home/<user>/sws/`).

Additionally, logging can be enabled as an option with the `-l` switch, and the log entries are formatted according to a modified version of Apache's "common" format.

# NOTES

Since this is meant to be a very simple server operating on a restricted subset of HTTP/1.0, there are many features relating to both functionality and modern web security which are not implemented. This program should NEVER be used as a production web server - if you need one of these, please look into nginx, Apache, or another popular and battle-tested server implementation.

The supported options and general structure are derived from Jan Schaumann's
CS 631 course, [Advanced Programming in the UNIX Environment](https://stevens.netmeister.org/631/).

# KNOWN ISSUES
