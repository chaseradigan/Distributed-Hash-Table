
# MP2 Fault-Tolerant Key-Value Store

## Local testing


Check the run procedure in `KVStoreTester.sh`. You can run this test suite locally like this:

```
$ chmod +x KVStoreTester.sh
$ ./KVStoreTester.sh
```

The `chmod` part only needs to be done once to allow the script to be executed. It's okay if the `chmod` command gives an error message. In that case you can do this instead:

```
$ bash ./KVStoreTester.sh
```

### How do I run the CRUD tests separately?

First, compile your project:

```
$ make clean
$ make
```

Then use one of these invocations:

```
$ ./Application ./testcases/create.conf
$ ./Application ./testcases/delete.conf
$ ./Application ./testcases/read.conf
$ ./Application ./testcases/update.conf
```

You may need to do `make clean && make` in between tests to make sure you have a clean run.
