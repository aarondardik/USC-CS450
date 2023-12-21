all: serverM.c serverCE.c serverEE.c client.c
	gcc -Wall -o serverM -g serverM.c
	gcc -Wall -o serverCE -g serverCE.c
	gcc -Wall -o serverEE -g serverEE.c
	gcc -Wall -o client -g client.c

serverM: serverM.c
	gcc -Wall -o serverM -g serverM.c

serverCE: serverCE.c
	gcc -Wall -o serverCE -g serverCE.c

serverEE: serverEE.c
	gcc -Wall -o serverEE -g serverEE.c

client: client.c
	gcc -Wall -o client -g client.c

clean:
	rm -f  serverM serverCE serverEE client

