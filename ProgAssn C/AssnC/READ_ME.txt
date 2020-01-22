I replaced all read and writes with the SSL equivilants. 
Removed  .pem from the certs and changed code to reflect such changes. 
If change back to .pem, go into directory server and serch key and certificate and add .pem if you so choose

COMPILE INSTRUCTIONS:
gcc -Wall -o server SSL_server.c -L/usr/lib -lssl -lcrypto
gcc -Wall -o client SSL_client.c -L/usr/lib -lssl -lcrypto
gcc -Wall -o dir SSL_dir.c -L/usr/lib -lssl -lcrypto

HOW TO RUN:
IN 3 TERMINALS, DO EACH CMD IN ONE TERMINAL IN THIS ORDER
./dir
./server NAME_OF_SERVER PORT_NUM
./client