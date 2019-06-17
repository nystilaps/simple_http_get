About http_get
--------------

Simple console http downloader.
Functionality is very basic: it downloads a file and outputs it to stdout, printing status and error messagers to stderr.
It is capable of handling no more than 10 redirects.


Usage
-----

To download a file from URL run:

`$./http_get 'url' > file`


Build
-----

To build run the following commands:

```
$ mkdir build
$ cd build
$ cmake ..
$ make
```
