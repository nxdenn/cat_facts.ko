## cat_facts.ko

Simple Linux kernel module that creates a device file at `/dev/catfacts`, returning cat facts when reading from it.  

### Usage
```sh
$ make all
$ insmod cat_facts.ko
$ cat /dev/catfacts
 A cat cannot see directly under its nose. This is why the cat cannot seem to find tidbits on the floor.
```
