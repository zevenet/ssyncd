# [Zevenet ssyncd](https://www.zevenet.com)
Enable replication of http sessions data and recent match sessions tables on Debian Gnu/Linux (Kernel version >= 3.2).

# Features
- Command line control for manipulating ssyncd stored data.
- Recent match tables replication
- HTTP sessions replication, includes support for Zevenet Pound session replication

## Installation and runtime
Ssyncd has been tested in stable Debian distributions: Wheezy, Jessie and Stretch

### Prerequisites
-  Patched xt_recent module (required) :
	- Required to enable writing of sessions data to recent match sessions tables. Under src/kmodule can be found patched xt recent modules
	 for stable debian kernel versions 3.2.93 (Wheezy) , 3.16.43 (Jessie) and 4.9.13 (Stretch).
-  Zevenet Pound (required):
	- Required to enable HTTP session replication and it's necessary that Pound process started with session synchronization enabled (option "-s").

### Build dependencies
- Cmake.
- C++11 compliant compiler.
- Google Re2 library (https://github.com/google/re2).

### Build 
```
git clone https://github.com/zevenet/ssyncd.git
cd ssyncd/bin
cmake ../
make
```

### Usage

Ssyncd has two modes of functioning, as master mode or as backup mode; master is the source of sessions data to be sent to backup node.
```
Usage: ssyncd -[MB] [adp]
    
Examples
    ssyncd -M [-p 7777]
    ssyncd -B [-a 127.0.0.1 -p 7777]

Commands: Either long or short options are allowed.
    -M  --master                        start master node
    -B  --backup                        start backup node

Options:
    -d  --daemon                        run ssyncd as daemon
    -a  --address   [master address]    master node address
    -p  --port      [master port]       master listening port
```
### Command line interface
Ssyncd is managed by a commands interface tools named ssyncdctl, and provide the following options:

- In master mode:
    - Start listening to Pound http sessions updates:

        ```# ssyncdctl start http [farm name]```
    - Start listening to recent sessions tables updates:

        ```# ssyncdctl start recent```

- Show ssyncd http sessions data:

    ```# ssyncdctl show http ```

- Show ssyncd recent tables sessions data:

    ```# ssyncdctl show recent ```

- In backup mode:
    - Write http sessions data to Pound sessions tables:

        ```	# ssyncdctl write http ```
	- Write recent tables session data to kernel tables:

		```# ssyncdctl write recent```

- Exit ssyncd process:

    ```# ssyncdctl quit```

## How to Contribute
All reported bugs, new features and patches are welcomed.

### Reporting
Please use the [GitHub project Issues](https://github.com/zevenet/ssyncd/issues) to report any issue or bug with the software. Try to describe the problem and a way to reproduce it. It'll be useful to attach the service and network configurations as well as system and services logs.


### [www.zevenet.com](https://www.zevenet.com)

