# Simple HTTP proxy server
My implementation of async http proxy server for the C++ course in ITMO University.  
It was developed and tested under OS X.

## Details
- kqueue for async I/O
- multi-threaded DNS resolver
- simple cache for responses that have ETag
