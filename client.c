#include "common.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define BUFSZ 1024

void usage(int argc, char **argv) {
	printf("usage: %s <server IP> <server port>\n", argv[0]);
	printf("example: %s 127.0.0.1 51511\n", argv[0]);
	exit(EXIT_FAILURE);
}

int main(int argc, char **argv) {
	if (argc < 3) {
		usage(argc, argv);
	}

	struct sockaddr_storage storage;
	if (0 != addrparse(argv[1], argv[2], &storage)) {
		usage(argc, argv);
	}

	int s;
	s = socket(storage.ss_family, SOCK_STREAM, 0);
	if (s == -1) {
		logexit("socket");
	}
	struct sockaddr *addr = (struct sockaddr *)(&storage);
	if (0 != connect(s, addr, sizeof(storage))) {
		logexit("connect");
	}

	char addrstr[BUFSZ];
	addrtostr(addr, addrstr, BUFSZ);

	char buf[BUFSZ];
	while (1) {
		memset(buf, 0, BUFSZ);
		printf("> ");
		fgets(buf, BUFSZ-1, stdin);

		// Remove \0 antes de realizar o envio da mensagem
		strtok(buf, "\0");
		size_t count = send(s, buf, strlen(buf)+1, 0);
		if (count != strlen(buf)+1) {
			logexit("send");
		}
	
		memset(buf, 0, BUFSZ);
		unsigned total = 0;
	
		while(1) {
			count = recv(s, buf + total, BUFSZ - total, 0);
			total += count;
			
			// Sem mensagem ou chegou ao final dela
			if (count == 0 || buf[total-1] == '\n') {
				// Conexao fechada
				break;
			}
		}

		// Conexao fechada pelo servidor
		if (!count) {
			break;
		}

		printf("< ");
		strtok(buf, "\n");
		puts(buf);
	}

	close(s);
	exit(EXIT_SUCCESS);
}