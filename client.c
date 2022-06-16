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

#define BUFSZ 40
#define MAX_CLIENTS 15

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
			exit(EXIT_SUCCESS);
		}

		strtok(buf, "\n");
		/*
		puts("--------------");
		puts(buf);
		puts("--------------");
		*/
	
		if (strlen(buf)>1) {
			strncpy(substring,buf,2);
			substring[2] = '\0';
			int command = atoi(substring);
			int identifier_equipament_i = 0;
			switch (command)
			{
				case 3:
					/* code */
					strncpy(substring,buf + 6,2);
					substring[2] = '\0';
					if (my_identifier == 0) {
						my_identifier = atoi(substring);
						if (my_identifier > 9) {
							snprintf(message_print, BUFSZ, "New ID: %d", my_identifier);
						} else {
							snprintf(message_print, BUFSZ, "New ID: 0%d", my_identifier);
						}
					} else{
						if (atoi(substring) > 9) {
							snprintf(message_print, BUFSZ, "Equipament %d added", atoi(substring));
						} else {
							snprintf(message_print, BUFSZ, "Equipament 0%d added", atoi(substring));
						}
						clients[(atoi(substring)-1)] = 1;
					}
					puts(message_print);
					break;
				case 7:
					/* code */
					strncpy(substring,buf + 6,2);
					substring[2] = '\0';
					switch (atoi(substring))
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
				case 8:
					strncpy(substring,buf+4,2);
					substring[2] = '\0';
					if (atoi(substring) == my_identifier) {
						snprintf(message_print, BUFSZ, "Successful removal");
						puts(message_print);
						close(cdata->socket);
						exit(EXIT_SUCCESS);
					} else {
						clients[(atoi(substring)-1)] = 0;
						if (atoi(substring)>9) {
							snprintf(message_print, BUFSZ, "Equipament %d removed", atoi(substring));
						} else {
							snprintf(message_print, BUFSZ, "Equipament 0%d removed", atoi(substring));
						}
						puts(message_print);
					}
					break;
				
				case 5:
					strncpy(substring,buf+2,2);
                    substring[2] = '\0';
                    identifier_equipament_i = atoi(substring);
                    strncpy(substring,buf+4,2);
                    substring[2] = '\0';
					puts("requested information");
					srand(time(NULL));
					int value = rand()%10;
					if (my_identifier > 9 && identifier_equipament_i > 9) {
						if (value > 9){
							snprintf(buf, BUFSZ, "06%d%d%d", my_identifier, identifier_equipament_i, value);
						} else {
							snprintf(buf, BUFSZ, "06%d%d0%d", my_identifier, identifier_equipament_i, value);
						}
						strtok(buf, "\0");
					} else if (my_identifier < 9 && identifier_equipament_i < 9) {
						if (value > 9){
							snprintf(buf, BUFSZ, "060%d0%d%d", my_identifier, identifier_equipament_i, value);
						} else {
							snprintf(buf, BUFSZ, "060%d0%d0%d", my_identifier, identifier_equipament_i, value);
						}
						strtok(buf, "\0");
					} else if (my_identifier > 9) {
						if (value > 9){
							snprintf(buf, BUFSZ, "06%d0%d%d", my_identifier, identifier_equipament_i, value);
						} else {
							snprintf(buf, BUFSZ, "06%d0%d0%d", my_identifier, identifier_equipament_i, value);
						}
						strtok(buf, "\0");
					} else {
						if (value > 9){
							snprintf(buf, BUFSZ, "060%d%d%d", my_identifier, identifier_equipament_i, value);
						} else {
							snprintf(buf, BUFSZ, "060%d%d0%d", my_identifier, identifier_equipament_i, value);
						}
						strtok(buf, "\0");
					}
					send(cdata->socket, buf, strlen(buf)+1, 0);
					break;
				
				case 6:
					strncpy(substring,buf+2,2);
                    substring[2] = '\0';
                    int identifier_equipament_origin = atoi(substring);
                    strncpy(substring,buf+4,2);
                    substring[2] = '\0';
                    int identifier_equipament_receive = atoi(substring);
                    strncpy(substring,buf+6,2);
                    substring[2] = '\0';
                    int value_read = atoi(substring);
					if (my_identifier == identifier_equipament_receive) {
						if (identifier_equipament_origin > 9) {
							snprintf(message_print, BUFSZ, "Value from %d: %d.00", identifier_equipament_origin, value_read);
						} else {
							snprintf(message_print, BUFSZ, "Value from 0%d: %d.00", identifier_equipament_origin, value_read);
						}
						puts(message_print);
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
	
	char substring[BUFSZ];

	while (1) {
		memset(buf, 0, BUFSZ);
		fgets(buf, BUFSZ-1, stdin);

		// Remove \0 antes de realizar o envio da mensagem
		strtok(buf, "\0");

		if (strlen(buf)>1) {
			if (strcmp(buf, "close connection\n") == 0 || strcmp(buf, "close connection") == 0) {
				if (my_identifier > 9) {
					snprintf(buf, BUFSZ, "02%dxxxx", my_identifier);
					strtok(buf, "\0");
				} else {
					snprintf(buf, BUFSZ, "020%dxxxx", my_identifier);
					strtok(buf, "\0");
				}
				size_t count = send(s, buf, strlen(buf)+1, 0);
				if (count != strlen(buf)+1) {
					logexit("send");
				}

				// Conexao fechada pelo servidor
				if (!count) {
					break;
				}
			}

			if (strcmp(buf, "list equipment\n") == 0 || strcmp(buf, "list equipment") == 0) {
				int has_print = 0;
				for (int i=0; i<MAX_CLIENTS; i++) {
					if (clients[i] != 0) {
						if (my_identifier != (i+1)) {
							if (has_print == 0) {
								if (i>9) {
									has_print = 1;
									printf("%d", (i+1));
								} else {
									has_print = 1;
									printf("0%d", (i+1));
								}
							} else {
								if (i>9) {
									has_print = 1;
									printf(" %d", (i+1));
								} else {
									has_print = 1;
									printf(" 0%d", (i+1));
								}

							}
						}
					}
				}
				printf("\n");
			}
			
			strncpy(substring,buf,25);
			substring[24] = '\0';
			if (strcmp(substring, "request information from \n") == 0 || strcmp(substring, "request information from") == 0) {
				strncpy(substring,buf+25,2);
				substring[2] = '\0';
				if (my_identifier > 9 && atoi(substring) > 9) {
					snprintf(buf, BUFSZ, "05%d%dxx", my_identifier, atoi(substring));
					strtok(buf, "\0");
				} else if (my_identifier < 9 && atoi(substring) < 9) {
					snprintf(buf, BUFSZ, "050%d0%dxx", my_identifier, atoi(substring));
					strtok(buf, "\0");
				} else if (my_identifier > 9) {
					snprintf(buf, BUFSZ, "05%d0%dxx", my_identifier, atoi(substring));
					strtok(buf, "\0");
				} else {
					snprintf(buf, BUFSZ, "050%d%dxx", my_identifier, atoi(substring));
					strtok(buf, "\0");
				}
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
	}

	close(s);
	exit(EXIT_SUCCESS);
}