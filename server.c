#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MAX_CLIENTS	100
#define BUFLEN 256

typedef struct
{
	char lastname[13];
	char firstname[13];
	int card_number;
	int pin;
	char passwd[9];
	double money;
	char blocked;
	char session;

} Client;

typedef struct 
{
	Client *receiver;
	double money;
	int card_number;

} Transfer;

typedef struct
{
	Client *client;
	Transfer *transfer;
	int last_attempt;
	char attempts;
	char is_connected;

} Session;

typedef struct
{
	Client *client;
	int port;
	int card_number;

} Unlock;

void error(char *msg)
{
    perror(msg);
    exit(1);
}

/* Executa comanda de login a unui client */
void login(int client, Session *sessions, Client *all_clients, char *full_command, int nb_clients)
{
	int card_number, pin;
	char is_client = 0;
	char send_msg[BUFLEN];
	char *parser = strtok(full_command, " ");
		
	parser = strtok(NULL, " ");
	card_number = (int) strtol(parser, (char **)NULL, 10);
		
	parser = strtok(NULL, " ");
	pin = (int) strtol(parser, (char **)NULL, 10);

	for (int i = 0; i < nb_clients; i++) {
		if (all_clients[i].card_number == card_number) {
			is_client = 1;

			if (all_clients[i].pin == pin) {
				if (all_clients[i].session == 0) {
					if (all_clients[i].blocked == 1) {
						send(client, "IBANK> −5 : Card blocat", sizeof("IBANK> −5 : Card blocat"), 0);

					} else {
						sessions[client].client = &all_clients[i];
						sessions[client].client->session = client;
						sessions[client].attempts = 0;
						sessions[client].last_attempt = 0;
				
						sprintf(send_msg, "IBANK> Welcome %s %s!", sessions[client].client->firstname, sessions[client].client->lastname);
						send(client, send_msg, sizeof(send_msg), 0);
					}

				} else {
					send(client, "IBANK> Sesiune deja deschisa", sizeof("IBANK> Sesiune deja deschisa"), 0);
				}
				
			} else if (sessions[client].attempts >= 2 && sessions[client].last_attempt == all_clients[i].card_number) {
				send(client, "IBANK> −5 : Card blocat", sizeof("IBANK> −5 : Card blocat"), 0);
				sessions[client].attempts++;
				all_clients[i].blocked = 1;
			
			} else if (sessions[client].last_attempt == all_clients[i].card_number) {
				send(client, "IBANK> −3 : Pin gresit", sizeof("IBANK> −3 : Pin gresit"), 0);
				sessions[client].attempts++;

			} else if (sessions[client].attempts < 2) {
				send(client, "IBANK> −3 : Pin gresit", sizeof("IBANK> −3 : Pin gresit"), 0);
				sessions[client].attempts = 1;
				sessions[client].last_attempt = all_clients[i].card_number;
			}

			break;
		}
	}

	if (is_client == 0) {
		send(client, "IBANK> −4 : Numar card inexistent", sizeof("IBANK> −4 : Numar card inexistent"), 0);
	}
}

/* Executa comanda de logout a unui client */
void logout(int client, Session *sessions)
{
	if (sessions[client].client == NULL) {
		send(client, "IBANK> -1 : Clientul nu este autentificat", sizeof("IBANK> -1 : Clientul nu este autentificat"), 0);
		return;
	}

	sessions[client].client->session = 0;
	sessions[client].client = NULL;
	sessions[client].attempts = 0;
	sessions[client].last_attempt = 0;

	send(client, "IBANK> Clientul a fost deconectat", sizeof("IBANK> Clientul a fost deconectat"), 0);
}

/* Executa comanda de listsold a unui client */
void listsold(int client, Session *sessions)
{
	if (sessions[client].client == NULL) {
		send(client, "IBANK> -1 : Clientul nu este autentificat", sizeof("IBANK> -1 : Clientul nu este autentificat"), 0);
		return;
	}

	char buffer[BUFLEN];
	sprintf(buffer, "IBANK> %.2f", sessions[client].client->money);

	send(client, buffer, sizeof(buffer), 0);
}

/* Executa comanda de transfer a unui client */
void transfer(int client, Session *sessions, Client *all_clients, char *full_command, int nb_clients)
{
	if (sessions[client].client == NULL) {
		send(client, "IBANK> -1 : Clientul nu este autentificat", sizeof("IBANK> -1 : Clientul nu este autentificat"), 0);
		return;
	}

	int card_number, i;
	double money;
	char is_client = 0;
	char *parser = strtok(full_command, " ");
	char recv_msg[BUFLEN] = {0};
	char send_msg[BUFLEN] = {0};
	Client *receiver = NULL;

	parser = strtok(NULL, " ");
	card_number = (int) strtol(parser, (char **)NULL, 10);
	
	parser = strtok(NULL, " ");
	money = (int) strtol(parser, (char **)NULL, 10);

	for (i = 0; i < nb_clients; i++) {
		if (all_clients[i].card_number == card_number) {
			is_client = 1;
			receiver = &all_clients[i];
			break;
		}
	}

	if (is_client == 0) {
		send(client, "IBANK> -4 : Numar card inexistent", sizeof("IBANK> -4 : Numar card inexistent"), 0);
	
	} else if (money > sessions[client].client->money) {
		send(client, "IBANK> -8 : Fonduri insuficiente", sizeof("IBANK> -4 : Fonduri insuficiente"), 0);
	
	} else {
		Transfer transfer;
		transfer.receiver = &all_clients[i];
		transfer.money = money;
		transfer.card_number = card_number;

		sessions[client].transfer = &transfer;

		sprintf(send_msg, "IBANK> Transfer %.2f catre %s %s? [ y/n ]", money, receiver->lastname, receiver->firstname);
		send(client, send_msg, sizeof(send_msg), 0);
	}
}

/* Executa comanda de finalizare a unui transfer */
void execute_transfer(int client, Session *sessions, Client *all_clients, char *buffer, int nb_clients)
{
	if (buffer[0] == 'y') {
		sessions[client].transfer->receiver->money += sessions[client].transfer->money;
		sessions[client].client->money -= sessions[client].transfer->money;
		sessions[client].transfer = NULL;

		send(client, "IBANK> Transfer realizat cu succes", sizeof("IBANK> Transfer realizat cu succes"), 0);

	} else {
		sessions[client].transfer = NULL;

		send(client, "IBANK> -9 : Operatie anulata", sizeof("IBANK> -9 : Operatie anulata"), 0);
	}
}

/* Executa comanda de unlock a unui client */
void unlock(int client, Unlock *unlock_clients, Session *sessions, Client *all_clients, char *full_command, int nb_clients, struct sockaddr_in cl_sockaddr)
{
	char is_client = 0;
	int card_number, i, j;
	Client *card;
	
	int port = ntohs(cl_sockaddr.sin_port);
	char *parser = strtok(full_command, " ");
	
	if (strcmp(parser, "unlock") == 0) {
		parser = strtok(NULL, " ");
		card_number = (int) strtol(parser, (char **)NULL, 10);

		for (i = 0; i < nb_clients; i++) {
			if (all_clients[i].card_number == card_number) {
				is_client = 1;
				card = &all_clients[i];
				break;
			}
		}

		if (is_client == 0) {
			sendto(client, "UNLOCK> -4 : Numar card inexistent", sizeof("UNLOCK> -4 : Numar card inexistent"), 0, (struct sockaddr *) &cl_sockaddr, sizeof(struct sockaddr));	
		
		} else if (card->blocked == 0) {
			sendto(client, "UNLOCK> -6 : Operatie esuata", sizeof("UNLOCK> -6 : Operatie esuata"), 0, (struct sockaddr *) &cl_sockaddr, sizeof(struct sockaddr));
		
		} else {
			for (i = 0; i < MAX_CLIENTS; i++) {
				if (unlock_clients[i].port == 0) {
					break;
				}
			}
			unlock_clients[i].port = port;
			unlock_clients[i].card_number = card_number;
			unlock_clients[i].client = card;

			sendto(client, "UNLOCK> Trimite parola secreta", sizeof("UNLOCK> Trimite parola secreta"), 0, (struct sockaddr *) &cl_sockaddr, sizeof(struct sockaddr));
		}

	} else {		
		for (i = 0; i < MAX_CLIENTS; i++) {
			if (unlock_clients[i].port == port) {
				break;
			}
		}

		char copy[BUFLEN];
		sprintf(copy, "%s\n", unlock_clients[i].client->passwd);
		
		if (strcmp(parser, copy) == 0) {
			unlock_clients[i].client->blocked = 0;
			unlock_clients[i].port = 0;
			
			sendto(client, "UNLOCK> Card deblocat", sizeof("UNLOCK> Card deblocat"), 0, (struct sockaddr *) &cl_sockaddr, sizeof(struct sockaddr));
		
		} else {
			sendto(client, "UNLOCK> -7 : Deblocare esuata", sizeof("UNLOCK> -7 : Deblocare esuata"), 0, (struct sockaddr *) &cl_sockaddr, sizeof(struct sockaddr));
		}
	}
}

int main(int argc, char *argv[])
{
	if (argc < 2) {
        fprintf(stderr,"Folosire %s port_server nume_fisier\n", argv[0]);
        exit(1);
    }

	/*Variabile folosite */
	int nb_clients, n, i, j, last_login, blocked_card_number;
	char buffer[BUFLEN];
	Session sessions[MAX_CLIENTS];
	Unlock unlock_clients[MAX_CLIENTS];

	for (int i = 0; i < MAX_CLIENTS; i++) {
		unlock_clients[i].port = 0;
	}

	/* Citirea datelor din fisier */
	FILE *f = fopen(argv[2], "rt");
	if (f == NULL) {
		error("Eroare la deschiderea fisierului");
	}

	fscanf(f,"%d", &nb_clients);

    Client *all_clients = malloc(nb_clients * sizeof(Client));
	if (all_clients == NULL) {
		error("Eroare la alocarea memoriei");
	}
    
	for (i = 0; i < nb_clients; i++){
        fscanf(f, "%s", all_clients[i].firstname);
        fscanf(f, "%s", all_clients[i].lastname);
        fscanf(f, "%d", &all_clients[i].card_number);
        fscanf(f, "%d", &all_clients[i].pin);
        fscanf(f, "%s", all_clients[i].passwd);
		fscanf(f, "%lf", &all_clients[i].money);
		all_clients[i].blocked = 0;
		all_clients[i].session = 0;
    }

	/* Se realizeaza conexiunea pentru UDP */
	struct sockaddr_in my_sockaddr, cl_sockaddr;
	int addr_len = sizeof(struct sockaddr);

	/* Deschidere socket UDP */
    int sockfd_udp = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd_udp == -1) {
		error("Eroare la deschiderea socket-ului UDP");
	}

	/* Setare struct sockaddr_in pentru a specifica unde trimit datele */
	my_sockaddr.sin_family = AF_INET;
	my_sockaddr.sin_port = htons(atoi(argv[1]));
	inet_aton("127.0.0.1", &(my_sockaddr.sin_addr));
	bzero(&(my_sockaddr.sin_zero),8);

	/* Legare proprietati de socket */    	
	if (bind(sockfd_udp, (struct sockaddr*) &my_sockaddr, sizeof(struct sockaddr)) < 0) {
		error("Eroare la legarea de aocket-ul UDP");
	}

	/* Se realizeaza conexiunea pentru TCP */
	int newsockfd, portno, clilen;
    struct sockaddr_in serv_addr, cli_addr;

	int sockfd_tcp = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd_tcp == -1) {
       error("Eroare la deschiderea socket-ului UDP");
	}

	/* Setare struct sockaddr_in pentru a specifica unde trimit datele */
    memset((char *) &serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(atoi(argv[1]));

	/* Legare proprietati de socket */ 
    if (bind(sockfd_tcp, (struct sockaddr *) &serv_addr, sizeof(struct sockaddr)) < 0) {
    	error("ERROR on binding");
	}
 
    listen(sockfd_tcp, MAX_CLIENTS);
    
	/* Organizarea file-descriptor-ilor */
	int fdmax;

    fd_set read_fds;
    fd_set tmp_fds; 
    
    FD_ZERO(&read_fds);
    FD_ZERO(&tmp_fds);

    FD_SET(0, &read_fds);
	FD_SET(sockfd_tcp, &read_fds);
	FD_SET(sockfd_udp, &read_fds);
	
	fdmax = sockfd_tcp > sockfd_udp ? sockfd_tcp : sockfd_udp;

	/* Primirea comenzilor si trimiterea lor la client */
	while (1) {
		tmp_fds = read_fds;
 
		if (select(fdmax + 1, &tmp_fds, NULL, NULL, NULL) == -1) {
			error("Eroare la realizarea selectului");
		}

		for(i = 0; i <= fdmax; i++) {
			if (FD_ISSET(i, &tmp_fds)) {

				/* Cazul in care se primesc date de la tastatura
				 * si se trimite comanda de quit la clienti
				 */
				if (i == 0) {
					memset(buffer, 0, BUFLEN);
					fgets(buffer, BUFLEN, stdin);

					if (strcmp(buffer, "quit\n") == 0) {
						for (int j = 0; j <= fdmax; j++) {
							if (sessions[j].is_connected == 1 && j != sockfd_tcp && i != sockfd_udp) {

								send(j, "quit", sizeof("quit"), 0);
								close(j);								
							}
						}

						close(sockfd_tcp);
						close(sockfd_udp);
						fclose(f);
										
						return 0;
					}
			
				/* Cazul in care se primesc date prin socket-ul UDP */
				} else if (i == sockfd_udp) {
					memset(buffer, 0, BUFLEN);
					memset(&cl_sockaddr, 0, sizeof(cl_sockaddr));

					if ((n = recvfrom(sockfd_udp, buffer, BUFLEN, 0, (struct sockaddr*) &cl_sockaddr, &addr_len)) <= 0) {
						if (n == 0) {
							printf("Serverul a fost inchis\n");

						} else {
							error("Eroare la primirea datelor prin UDP\n");
						}

						close(i);
						FD_CLR(i, &read_fds);

					} else {
						unlock(i, unlock_clients, sessions, all_clients, buffer, nb_clients, cl_sockaddr);
					}

				/* Se creeaza o noua instanta a conexiunii TCP */
				} else if (i == sockfd_tcp) {
					clilen = sizeof(cli_addr);

					if ((newsockfd = accept(sockfd_tcp, (struct sockaddr *)&cli_addr, &clilen)) == -1) {
						error("Eroare la accept in TCP");

					} else {
						FD_SET(newsockfd, &read_fds);
						
						if (newsockfd > fdmax) { 
							fdmax = newsockfd;
						}

						sessions[newsockfd].client = NULL;
						sessions[newsockfd].transfer = NULL;
						sessions[newsockfd].attempts = 0;
						sessions[newsockfd].last_attempt = 0;
						sessions[newsockfd].is_connected = 1;
					}

					printf("Un nou client conectat de la %s, port %d, socket_client %d\n", 
								inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port), newsockfd);

				/* Cazul in care se primesc comenzi prin socket-ul TCP */
				} else {
					memset(buffer, 0, BUFLEN);

					if ((n = recv(i, buffer, sizeof(buffer), 0)) <= 0) {
						if (n == 0) {
							printf("Clientul a fost inchis\n");

						} else {
							error("Eroare la primirea datelor prin TCP\n");
						}
						
						close(i);
						FD_CLR(i, &read_fds);
					
					} else {
						char copy[BUFLEN];
						memcpy(copy, buffer, sizeof(buffer));
						char *command = strtok(copy, " ");
						
						if (sessions[i].transfer != NULL) {
							execute_transfer(i, sessions, all_clients, command, nb_clients);

						} else if (strcmp(command, "login") == 0) {
							login(i, sessions, all_clients, buffer, nb_clients);

						} else if (strcmp(command, "logout\n") == 0) {
							logout(i, sessions);
						
						} else if (strcmp(command, "listsold\n") == 0) {
							listsold(i, sessions);
						
						} else if (strcmp(command, "transfer") == 0) {
							transfer(i, sessions, all_clients, buffer, nb_clients);
						
						} else if (strcmp(command, "quit\n") == 0) {
							sessions[i].is_connected = 0;
							sessions[i].last_attempt = 0;
							sessions[i].attempts = 0;

							if (sessions[i].client != NULL) {
								sessions[i].client->session = 0;
								sessions[i].client = NULL;
							}

							close(i);
							FD_CLR(i, &read_fds);

						} else {	
							printf("\n");
						}
					}
				} 
			}
		}
    }

    close(sockfd_tcp);
	close(sockfd_udp);
	fclose(f);
   
    return 0; 
}


