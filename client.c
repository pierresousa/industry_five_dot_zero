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
#define MAX_CLIENTS 2

int send_message_flag = 0;
int receive_message_flag = 0;
int my_identifier = 0;

struct client_data {
    int socket;
};

int clients[MAX_CLIENTS];

void usage(int argc, char **argv) {
	printf("usage: %s <server IP> <server port>\n", argv[0]);
	printf("example: %s 127.0.0.1 51511\n", argv[0]);
	exit(EXIT_FAILURE);
}

void * message_receive_thread(void *data) {
	struct client_data *cdata = (struct client_data *)data;
	char buf[BUFSZ];
	char message_print[BUFSZ];
	char substring[3];
    while (1) {
		memset(buf, 0, BUFSZ);
		memset(message_print, 0, BUFSZ);
		size_t count = recv(cdata->socket, buf, BUFSZ - 1, 0);
		
		// Conexao fechada pelo servidor
		if (!count) {
			close(cdata->socket);
			/* Restore old settings */
			exit(EXIT_SUCCESS);
		}

		strtok(buf, "\n");
		puts("--------------");
		puts(buf);
		puts("--------------");
		
		if (strlen(buf)>1) {
			strncpy(substring,buf,2);
			substring[2] = '\0';
			int command = atoi(substring);
			switch (command)
			{
				case 3:
					/* code */
					strncpy(substring,buf + 6,2);
					substring[2] = '\0';
					if (my_identifier == 0) {
						my_identifier = atoi(substring);
						snprintf(message_print, BUFSZ, "New ID: %d", my_identifier);
					} else{
						snprintf(message_print, BUFSZ, "Equipament %d added", atoi(substring));
						clients[(atoi(substring)-1)] = 1;
					}
					puts(message_print);
					break;
				case 7:
					/* code */
					strncpy(substring,buf + 6,2);
					substring[2] = '\0';
					int payload = atoi(substring);
					switch (payload)
					{
						case 1:
							snprintf(message_print, BUFSZ, "Equipament not found");
							puts(message_print);
							break;
						case 2:
							snprintf(message_print, BUFSZ, "Source equipament not found");
							puts(message_print);
							break;
						case 3:
							snprintf(message_print, BUFSZ, "Target equipament not found");
							puts(message_print);
							break;
						case 4:
							snprintf(message_print, BUFSZ, "Equipament limit exceeded");
							puts(message_print);
							break;
						default:
							break;
					}
					break;
				case 4:
					for (int count = 6; count<strlen(buf); count += 2) {
						strncpy(substring,buf + count,2);
						substring[2] = '\0';
						int payload = atoi(substring);
						if (payload>0 && payload <= 15) {
							clients[(payload-1)] = 1;
						}
					}
					break;
				
				default:
					break;
			}
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

	snprintf(buf, BUFSZ, "01xxxxxx");
	strtok(buf, "\0");
	send(s, buf, strlen(buf)+1, 0);

	while (my_identifier == 0)
	{
		continue;
	}

	for (int i=0; i<MAX_CLIENTS; i++) {
        clients[i] = 0;
    }
	
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