# libcutil
## Overview

Utility functions for C language. Features are described below:

- `basic data structures`: such as (singly|doubly)linked list, queue etc copied from [freebsd](http://www.freebsd.org/)
- `lru`: LRU(Least Recently Used) based on doubly linked list
- `hashmap`: Hashmap based on [go1.1](http://golang.org/)'s implementation (simplified AND modified for getting rid of go runtime dependencies)
- `slab`: slab allocator for fast memory management(malloc/free)
- `cache`: simple cache implementation with LRU

## Dependencies

* [Check](http://check.sourceforge.net/) for C Unit tests
* [CMake](http://www.cmake.org/) for building/testing/deploying C programs
* Ubuntu 16.04:
    * ``libsubunit-dev``

## Install dependencies

* For Ubuntu 16.04

```shell
$ sudo apt-get install cmake check libsubunit-dev
```

* For Ubuntu/Debian

```shell
$ sudo apt-get install cmake check
```

* For RHEL/CentOS

```shell
$ sudo yum install cmake check check-devel
```

## Build/Install libcutil

```shell
    git clone git://github.com/leo-project/libcutil.git
    cd libcutil
    mkdir build
    cd build
    cmake ..
    make
    make test
    sudo make install
```

## Sponsors

LeoProject/LeoFS is sponsored by [Rakuten, Inc.](http://global.rakuten.com/corp/) and supported by [Rakuten Institute of Technology](http://rit.rakuten.co.jp/).