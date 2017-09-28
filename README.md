
== Compile ==

    make clean
    make all


== Run netcat server and client ==

- Run netcatd:

    ./netcatd <port>

  For example:

    ./netcatd 8207 > myfileCopy

- Run netcat:
  
    ./netcat <server_ip> <server_port>

  For example:
   
    cat myfile | ./netcat 192.168.1.1 8207


== Run spdtest server and client ==

- Run spdtestd:

    ./spdtestd <port> <msg_size> <sleep_ms>

  For example:
    
    ./spdtestd 9207 8192 10

- Run spdtest:

    ./spdtest <server_ip> <server_port> <msg_size>

  For example:

    ./spdtest 192.168.1.1 9207 8192

  Note that the <msg_size> argument must be the same for spdtest
  client and server.


