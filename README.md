Sysinfo / Nictagadm
===================

`sysinfo` and `nictagadm` rewrites in C.

[`sysinfo`](https://smartos.org/man/1m/sysinfo) is a command on SmartOS that
outputs information about the current system as JSON.  It is currently a [bash
script](https://github.com/joyent/smartos-live/blob/master/src/sysinfo) and
can take long time to gather information (see PERFORMANCE below).

`sysinfo` relies on mostly external commands and reading various files, so
porting it to C is a relatively straight-forward task of replacing external
programs with library calls, and reading the data in directly to the program.
However, `sysinfo` relies on
[`nictagadm`](https://github.com/joyent/smartos-live/blob/master/src/nictagadm)
which itself is another bash script.  To remedy this, I've created `libnictag.so`
in this repo (in `common/nictag`) where, ideally, `sysinfo` can use to gather
this information, and `nictagadm` the command can just be a thin C wrapper around
the library.

Usage
-----

For `sysinfo`

    cd sysinfo
    make
    ./sysinfo | json

For `nictagadm`

    cd nictagadm
    make
    ./nictagadm list

Current State
-------------

### `sysinfo`

Based on my test machine (Vanilla SmartOS without Triton), there are only
a couple of key pieces of data missing.

```
# diff <(./sysinfo | json -k | json -a | sort) <(sysinfo | json -k | json -a | sort)
5a6
> CPU Virtualization
9a11
> Link Aggregations
20a23
> Virtual Network Interfaces
```

#### `CPU Virtalization`

This key is determined by running `isainfo -x`.  However, the command itself
relies on `cpuid_drv.h` being present, and according to this [Makefile](https://github.com/joyent/illumos-joyent/blob/master/usr/src/uts/common/sys/Makefile):

```
#
#   Note that the following headers are present in the kernel but
#   neither installed or shipped as part of the product:
#       cpuid_drv.h:        Private interface for cpuid consumers
#       unix_bb_info.h:     Private interface to kcov
#
```

#### `Link Aggregations` and `Virtual Network Interfaces`

I don't have any on the machine I'm testing with

### `nictagadm`

This command is seriously lacking, as well as `libnictag.h`... lot's of work needed here.

### `libnictag.so`

Should this even be a shared object? is this the best way to share code? i don't know.

this seriously needs work.

Performance
-----------

The main reason for this rewrite is performance.  `sysinfo` as a bash script is
aware of itself being slow to gather information, so it caches stored information
in a temp file (`/tmp/.sysinfo.json`) and under normal invocations will just exec
`cat` to print the cached data.

When a program updates something on the system, it must itself be aware of if it
should run `sysinfo -u` to update the cache file.

Also, `sysinfo -f` will force a regather of the information and skip the cache
completely.

Knowing this, we can see how long it takes to run:

### Reading the cached data

```
# ptime sysinfo >/dev/null

real        0.032801359
user        0.011272284
sys         0.019612098
```

### Gathering data from the system

```
# ptime sysinfo -f >/dev/null

real        1.193742762
user        0.369165330
sys         0.940171487
```

### Gathering data from the C version

```
# ptime ./sysinfo -f >/dev/null

real        0.027513249
user        0.007087713
sys         0.017696761
```

From this data alone it can be seen that the C version of `sysinfo` can gather,
format, and print the data faster than the bash script can exec `cat`.

License
-------

See individual files - all files will be released under the same licenses as
https://github.com/joyent/smartos-live
