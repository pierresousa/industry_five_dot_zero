#include "common.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#include <sys/socket.h>
#include <sys/types.h>

#define BUFSZ 40
#define MAX_CLIENTS 15

int num_clients_connected = 0;

void usage(int argc, char **argv) {
    printf("usage: %s <server port>\n", argv[0]);
    printf("example: %s 51511\n", argv[0]);
    exit(EXIT_FAILURE);
}

struct client_data {
    int csock;
    struct sockaddr_storage storage;
};

struct client_data *clients[MAX_CLIENTS];

void res_list (void *data) {
    struct client_data *cdata = (struct client_data *)data;
    char buf[BUFSZ];
    memset(buf, 0, BUFSZ);
    snprintf(buf, BUFSZ, "04xxxx");

    char identifier[3];
    for (int i=0; i<MAX_CLIENTS; i++) {
        if (clients[i] != NULL) {
            if(i>=9) {
                snprintf(identifier, 3, "%d", (i+1));
            } else {
                snprintf(identifier, 3, "0%d", (i+1));
            }
            strcat(buf, identifier);
        }
    }
    strtok(buf, "\0");
    send(cdata->csock, buf, strlen(buf), 0);
}

int insert_clients (void *data) {
    struct client_data *cdata = (struct client_data *)data;
    char buf[BUFSZ];
    memset(buf, 0, BUFSZ);

    if (num_clients_connected >= MAX_CLIENTS) {
        // Se limite excedido, nao ha IdEQ
        snprintf(buf, BUFSZ, "07xxxx04");
        strtok(buf, "\0");
        send(cdata->csock, buf, strlen(buf), 0);
        close(cdata->csock);
        return -1;
    }

    for (int i=0; i<MAX_CLIENTS; i++) {
        if (clients[i] == NULL) {
            clients[i] = cdata;
            num_clients_connected++;


            if(i>=9) {
                printf("Equipament %d added\n", (i+1));
            } else {
                printf("Equipament 0%d added\n", (i+1));
            }

            if(i>=9) {
                snprintf(buf, BUFSZ, "03xxxx%d", (i+1));
            } else {
                snprintf(buf, BUFSZ, "03xxxx0%d", (i+1));
            }
            strtok(buf, "\0");
            send(cdata->csock, buf, strlen(buf), 0);

            for (int j=0; j<MAX_CLIENTS; j++) {
                if (j != i && clients[j] != NULL) {
                    if(i>=9) {
                        snprintf(buf, BUFSZ, "03xxxx%d", (i+1));
                    } else {
                        snprintf(buf, BUFSZ, "03xxxx0%d", (i+1));
                    }
                    strtok(buf, "\0");
                    send(clients[j]->csock, buf, strlen(buf), 0);
                }
            }

            res_list(data);
            return 1;
        }
    }

    // Nem precisa desse return devido as consideracoes de projeto
    return 1;
}

void * client_thread(void *data) {
    struct client_data *cdata = (struct client_data *)data;
    struct sockaddr *caddr = (struct sockaddr *)(&cdata->storage);

    char caddrstr[BUFSZ];
    addrtostr(caddr, caddrstr, BUFSZ);

    char buf[BUFSZ];
    char message_print[BUFSZ];
	char substring[3];
    while (1) {
        memset(buf, 0, BUFSZ);
        size_t count = recv(cdata->csock, buf, BUFSZ - 1, 0);
        if (count == 0) {
            printf("[log] conexao fechada pelo cliente: %s\n", caddrstr);
            printf("[log] conexao fechada de forma invalida! nenhuma tratativa sera feita.\n");
            break;
        }
        
        // printf("[msg] %s, %d bytes: %s\n", caddrstr, (int)count, buf);

        if (strlen(buf)>1) {
			strncpy(substring,buf,2);
			substring[2] = '\0';
			int command = atoi(substring);
            int identifier = 0;
            int identifier_equipament_i = 0;
            int identifier_equipament_j = 0;

			switch (command)
			{
				case 1:
					/* code */
					if (insert_clients(data) == -1){
                        close(cdata->csock);
                        pthread_exit(EXIT_SUCCESS);
                    }
					break;
				
                case 2:
					/* code */
                    strncpy(substring,buf+2,2);
                    substring[2] = '\0';
                    identifier = atoi(substring);
                    if (clients[identifier-1] == NULL) {
                        snprintf(buf, BUFSZ, "07xxxx01");
                        strtok(buf, "\0");
                        send(cdata->csock, buf, strlen(buf), 0);
                    } else {
                        clients[identifier-1] = NULL;
                        if (identifier>9) {
                            snprintf(buf, BUFSZ, "08xx%d01", identifier);
                            strtok(buf, "\0");
                        } else {
                            snprintf(buf, BUFSZ, "08xx0%d01", identifier);
                            strtok(buf, "\0");
                        }
                        send(cdata->csock, buf, strlen(buf), 0);
                        close(cdata->csock);
                        if (identifier>9) {
                            snprintf(message_print, BUFSZ, "Equipament %d removed", identifier);
                        } else {
                            snprintf(message_print, BUFSZ, "Equipament 0%d removed", identifier);
                        }
                        puts(message_print);
                        for (int j=0; j<MAX_CLIENTS; j++) {
                            if (j != (identifier-1) && clients[j] != NULL) {
                                if(identifier>=9) {
                                    snprintf(buf, BUFSZ, "08xx%d01", identifier);
                                } else {
                                    snprintf(buf, BUFSZ, "08xx0%d01", identifier);
                                }
                                strtok(buf, "\0");
                                send(clients[j]->csock, buf, strlen(buf), 0);
                            }
                        }
                        pthread_exit(EXIT_SUCCESS);
                    }
					break;
				
                case 5:
					/* code */
					strncpy(substring,buf+2,2);
                    substring[2] = '\0';
                    identifier_equipament_i = atoi(substring);
                    strncpy(substring,buf+4,2);
                    substring[2] = '\0';
                    identifier_equipament_j = atoi(substring);
                    
                    if (clients[identifier_equipament_i-1] == NULL) {
                        snprintf(message_print, BUFSZ, "Equipament %d not found", identifier_equipament_i);
                        puts(message_print);
                        if (identifier_equipament_i > 9) {
                            snprintf(buf, BUFSZ, "07xx%d02", identifier_equipament_j);
                        } else {
                            snprintf(buf, BUFSZ, "07xx0%d02", identifier_equipament_j);
                        }
                        strtok(buf, "\0");
                        send(cdata->csock, buf, strlen(buf), 0);
                    } else {
                        if (clients[identifier_equipament_j-1] == NULL) {
                            snprintf(message_print, BUFSZ, "Equipament %d not found", identifier_equipament_j);
                            puts(message_print);
                            if (identifier_equipament_j > 9) {
                                snprintf(buf, BUFSZ, "07xx%d03", identifier_equipament_j);
                            } else {
                                snprintf(buf, BUFSZ, "07xx0%d03", identifier_equipament_j);
                            }
                            strtok(buf, "\0");
                            send(cdata->csock, buf, strlen(buf), 0);
                        } else {
                            send(clients[identifier_equipament_j-1]->csock, buf, strlen(buf), 0);
                        }

                    }

					break;
                
                case 6:
					/* code */
					strncpy(substring,buf+2,2);
                    substring[2] = '\0';
                    identifier_equipament_j = atoi(substring);
                    strncpy(substring,buf+4,2);
                    substring[2] = '\0';
                    identifier_equipament_i = atoi(substring);
                    
                    if (clients[identifier_equipament_j-1] == NULL) {
                        snprintf(message_print, BUFSZ, "Equipament %d not found", identifier_equipament_j);
                        puts(message_print);
                        if (identifier_equipament_j > 9) {
                            snprintf(buf, BUFSZ, "07xx%d02", identifier_equipament_i);
                        } else {
                            snprintf(buf, BUFSZ, "07xx0%d02", identifier_equipament_i);
                        }
                        strtok(buf, "\0");
                        send(cdata->csock, buf, strlen(buf), 0);
                    } else {
                        if (clients[identifier_equipament_j-1] == NULL) {
                            snprintf(message_print, BUFSZ, "Equipament %d not found", identifier_equipament_j);
                            puts(message_print);
                            if (identifier_equipament_j > 9) {
                                snprintf(buf, BUFSZ, "07xx%d03", identifier_equipament_j);
                            } else {
                                snprintf(buf, BUFSZ, "07xx0%d03", identifier_equipament_j);
                            }
                            strtok(buf, "\0");
                            send(cdata->csock, buf, strlen(buf), 0);
                        } else {
                            send(clients[identifier_equipament_i-1]->csock, buf, strlen(buf), 0);
                        }

                    }

					break;

				default:
					break;
			}
		}
    }
    
    close(cdata->csock);
    pthread_exit(EXIT_SUCCESS);
}

int main(int argc, char **argv) {
    if (argc < 2) {
        usage(argc, argv);
    }

    struct sockaddr_storage storage;
    if (0 != server_sockaddr_init("v4", argv[1], &storage)) {
        usage(argc, argv);
    }

    int s;
    s = socket(storage.ss_family, SOCK_STREAM, 0);
    if (s == -1) {
        logexit("socket");
    }

    int enable = 1;
    if (0 != setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int))) {
        logexit("setsockopt");
    }

    struct sockaddr *addr = (struct sockaddr *)(&storage);
    if (0 != bind(s, addr, sizeof(storage))) {
        logexit("bind");
    }

    if (0 != listen(s, 10)) {
        logexit("listen");
    }

    for (int i=0; i<MAX_CLIENTS; i++) {
        clients[i] = NULL;
    }

    char addrstr[BUFSZ];
    addrtostr(addr, addrstr, BUFSZ);
    printf("Ligado a %s, esperando conexoes\n", addrstr);

    while (1) {
        struct sockaddr_storage cstorage;
        struct sockaddr *caddr = (struct sockaddr *)(&cstorage);
        socklen_t caddrlen = sizeof(cstorage);

        int csock = accept(s, caddr, &caddrlen);
        if (csock == -1) {
            logexit("accept");
        }

        struct client_data *cdata = malloc(sizeof(*cdata));
        if (!cdata) {
            logexit("malloc");
        }
        cdata->csock = csock;
        memcpy(&(cdata->storage), &cstorage, sizeof(cstorage));

        pthread_t tid;
        pthread_create(&tid, NULL, client_thread, cdata);
    }

    exit(EXIT_SUCCESS);
}

