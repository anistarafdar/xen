# XEN

> interpreter for the XEN programming language

**Xen** (pronounced "zen"), is an interpreter for a Lisp-like programming language written in C for personal/educational purposes. As it's heavily inspired and based on the Lisp family of languages, it is very simple to use.

This project was written as an introduction to building programming languages, the Lisp family of languages, and C programming. 

## Getting Started

On **Unix**

'''console
cc -std=c99 -Wall xen.c mpc.c -ledit -lm -o xen
'''

On **Windows**

'''console
cc -std=c99 -Wall xen.c mpc.c -o xen
'''

## Links
http://buildyourownlisp.com/

https://github.com/gtr/ivy

http://hactar.port70.net/stutter/

## TODO
* provide a make file
* finish writing readme
* show off features in the readme
* add more useful features
* clean repo and codebase
* fix it for Windows
