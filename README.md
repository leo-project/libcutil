libcutil
========
Utility functions for C language. Features are described below.

- `basic data structures`: such as (singly|doubly)linked list, queue etc copied from [freebsd](http://www.freebsd.org/)
- `lru`: LRU(Least Recently Used) based on doubly linked list
- `hashmap`: Hashmap based on [go1.1](http://golang.org/)'s implementation (simplified AND modified for getting rid of go runtime dependencies)
- `slab`: slab allocator for fast memory management(malloc/free)
- `cache`: simple cache implementation with LRU

Dependencies
============
* [Check](http://check.sourceforge.net/) for C Unit tests
* [CMake](http://www.cmake.org/) for building/testing/deploying C programs

Install dependencies by package manager
=======================================
* deb

```shell
    sudo apt-get install cmake check
```

* rpm

```shell
    sudo yum install cmake check check-devel
```

Build/Install
=============
```shell
    git clone git://github.com/leo-project/libcutil.git
    cd libcutil
    cmake ..
    make
    make test
    sudo make install
```
