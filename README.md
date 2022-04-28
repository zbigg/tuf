# TUF - track used files 

`tuf` tracks used files by LD_PRELOADING wrappers to standard file access functions (open,exec,...)

## Usage examples

```shell
   $ ./tuf 'cat /etc/hosts | grep local'
   tuf: note, logging file access to /home/user/.tuf-13507.txt
   $ cat /home/user/.tuf-13507.txt
   open /dev/tty
   exec /bin/cat
   exec /bin/grep
   open /etc/hosts
```

## Installation

```
make
make install DESTDIR=${HOME}
```

## Usage

`tuf`:
```
tuf COMMAND - output goes to ./.tuf-<pid>.txt
tuf -o file COMMAND
```

`tuf-deps` - analyze what packages your toolchain depends

 Usage:
 ```
    tuf-deps [options] [TUF-OUTPUT-FILES]  - analyze previously collected trackings
    tuf-deps [options] --command 'COMMAND' - analyze particular COMMAND
```

Options:
```
-x  PATTERN
--exclude PATTERN   -- exclude files matching PATTERN

--output_mode MODE  -- select output mode

-c COMMAND
--command COMMAND   -- execute COMMAND and analyze deps of 
                        this command
```
Output modes:

 * `file_list` (default): outputs space separate tuple: file packager_type package root
 * `package_list`: files agreegated on "packager_type:package" string

 Example:
 ```
$ tuf -o mytuf.txt make
$ tuf-deps mytuf.txt
/usr/include/unistd.h rpm glibc-headers-2.5-58 /
/usr/include/bits/typesizes.h rpm glibc-headers-2.5-58 /
/bin/rm rpm coreutils-5.97-23.el5_4.2 /
/bin/bash rpm bash-3.2-24.el5 /
(...)

$ tuf-deps --output-mode package_list mytuf.txt
(...)
rpm:bash-3.2-24.el5 /bin/bash
rpm:coreutils-5.97-23.el5_4.2 (...) /bin/rm (...)
rpm:glibc-headers-2.5-58 (...) /usr/include/unistd.h /usr/include/bits/typesizes.h (...)
(...)
```

