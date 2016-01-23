Experimental Setup:
Two linux laptops connected over LAN.
Server side information:
Sector size = 512 bytes (command: lsblk -o NAME,PHY-SeC)
CPU cores = 2 (command: less /proc/cpuinfo)
cpu MHz         : 1547.753
cache size      : 3072 KB

Client side Information:
CPU cores: 2

Caches were dropped from server side

Instructions to execute code:
1. Edit the parameters at the top of the Makefile
2. Place the makefile in the same directory as the 2 .c files
3. Clear the caches using : sudo echo 3|sudo tee /proc/sys/vm/drop_caches
4. Run: make server (on server side)
	 make multi-client (on client side after running server)

The output at client side gives the Average throughput and average response time.

Note:
1. There are 10k files of 2 MB each.
2. Reads and writes on the socket are done in chunks of 1024B 
3. In fixed mode, a thread asks for file corresponding to its index-1, i.e. the 3rd thread created writes to server
'get files/foo2.txt'
4. Total time is measured at client starting from the point of creation of thread to the point where all the threads have exited.
