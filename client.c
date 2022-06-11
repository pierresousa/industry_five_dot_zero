#include "common.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#include <sys/socket.h>
#include <sys/types.h>

#include <termios.h>

#define BUFSZ 1024

int send_message_flag = 0;
int receive_message_flag = 0;

struct client_data {
    int socket;
};

void usage(int argc, char **argv) {
	printf("usage: %s <server IP> <server port>\n", argv[0]);
	printf("example: %s 127.0.0.1 51511\n", argv[0]);
	exit(EXIT_FAILURE);
}

void * message_receive_thread(void *data) {
	struct client_data *cdata = (struct client_data *)data;
	char buf[BUFSZ];
    while (1) {
		memset(buf, 0, BUFSZ);
		size_t count = recv(cdata->socket, buf, BUFSZ - 1, 0);
		
		// Conexao fechada pelo servidor
		if (!count) {
			break;
		}
		if (strlen(buf)>1) {
			strtok(buf, "\n");
			puts(buf);
		}
    }

    pthread_exit(EXIT_SUCCESS);
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
	struct client_data *cdata = malloc(sizeof(*cdata));
	if (!cdata) {
		logexit("malloc");
	}
	cdata->socket = s;
	pthread_t tid;
	pthread_create(&tid, NULL, message_receive_thread, cdata);

	char buf[BUFSZ];
	while (1) {
		memset(buf, 0, BUFSZ);
		fgets(buf, BUFSZ-1, stdin);

		// Remove \0 antes de realizar o envio da mensagem
		strtok(buf, "\0");

		if (strlen(buf)>1) {
			size_t count = send(s, buf, strlen(buf)+1, 0);
			if (count != strlen(buf)+1) {
				logexit("send");
			}

			// Conexao fechada pelo servidor
			if (!count) {
				break;
			}
		}
	}

	close(s);
	/* Restore old settings */
	exit(EXIT_SUCCESS);
}