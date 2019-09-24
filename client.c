#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>

#define BUFLEN 256

void error(char *msg)
{
    perror(msg);
    exit(0);
}

int main(int argc, char *argv[])
{
	/* Verificare folosire corecta a clientului */
	if (argc < 3) {
       fprintf(stderr,"Folosire %s adresa_server port_server\n", argv[0]);
       exit(1);
    }

	/* Variabile folosite */
	char last_command[BUFLEN], *command, buffer[BUFLEN] = {0}, on_transfer = 0, is_logged = 0;
	int n, i, last_attempt;

	/* Deschiderea fisierului de log al clientului */
	FILE *f;
	sprintf(buffer, "client-<%d>.log", getpid());

	f = fopen(buffer, "a+");
	if (f == NULL) { 
		error("Eroare la fisierul de log.\n");
	}

	/* Se realizeaza conexiunea pentru UDP */
	struct sockaddr_in to_station;
	int addr_len = sizeof(struct sockaddr);

	/* Deschidere socket UDP */
	int sockfd_udp = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd_udp == -1) {
		error("Eroare la deschiderea socket-ului UDP");
	}

	/* Setare struct sockaddr_in pentru a specifica unde trimit datele */
	to_station.sin_family = AF_INET;
	to_station.sin_port = htons(atoi(argv[2]));
	inet_aton(argv[1], &(to_station.sin_addr));
	bzero(&(to_station.sin_zero),8);

	/* Se realizeaza conexiunea pentru TCP */
    struct sockaddr_in serv_addr;
    struct hostent *server;

	/* Deschidere socket TCP */
	int sockfd_tcp = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd_tcp < 0) {
        error("Eroare la deschiderea socket-ului TCP");
	}

	/* Setare struct sockaddr_in pentru a specifica unde trimit datele */
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(atoi(argv[2]));
    inet_aton(argv[1], &serv_addr.sin_addr);
    
    if (connect(sockfd_tcp,(struct sockaddr*) &serv_addr,sizeof(serv_addr)) < 0) {
        error("Eroare la conectarea socket-ului TCP");    
	}
	
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

	/* Primirea comenzilor si trimiterea lor la server */
	while (1) {
		tmp_fds = read_fds;

		if (select(fdmax + 1, &tmp_fds, NULL, NULL, NULL) == -1) {
			error("Eroare la realizarea selectului");
		}

		for(i = 0; i <= fdmax; i++) {
			if (FD_ISSET(i, &tmp_fds)) {

				/* Cazul in care se primesc date de la tastatura
				 * si se trimit comenzile la server
				 */
				if (i == 0) {
					memset(buffer, 0, BUFLEN);
					fgets(buffer, BUFLEN - 1, stdin);

					char copy[BUFLEN];
					memcpy(copy, buffer, sizeof(buffer));
					command = strtok(copy, " ");

					if (strcmp(last_command, "transfer") == 0) {
						int send_check = send(sockfd_tcp, buffer, sizeof(buffer), 0);
						if (send_check == -1) {
							perror("Eroare la trimiterea comenzii prin TCP\n");
						}

						fprintf(f, "%s", buffer);
					
					} else if(strcmp(command, "transfer") == 0) {
						on_transfer = 1;

						int send_check = send(sockfd_tcp, buffer, sizeof(buffer), 0);
						if (send_check == -1) {
							perror("Eroare la trimiterea comenzii prin TCP\n");
						}

						fprintf(f, "%s", buffer);

					} else if (strcmp(command, "login") == 0) {
						if (is_logged == 1) {
							printf("> -2 : Sesiune deja deschisa\n");

						} else {
							command = strtok(NULL, " ");
							last_attempt = (int) strtol(command, (char **)NULL, 10);
					
							int send_check = send(sockfd_tcp, buffer, sizeof(buffer), 0);
							if (send_check == -1) {
								perror("Eroare la trimiterea comenzii prin TCP\n");
							}

							fprintf(f, "%s", buffer);
						}

					} else if (strcmp(command, "quit\n") == 0) {
						send(sockfd_tcp, command, sizeof(last_command), 0);
						fprintf(f, "%s", buffer);

						close(sockfd_tcp);
	 					close(sockfd_udp);
						fclose(f);
   
     					return 0; 

					} else if (strcmp(command, "logout\n") == 0) {
						is_logged = 0;

						int send_check = send(sockfd_tcp, buffer, sizeof(buffer), 0);
						if (send_check == -1) {
							perror("Eroare la trimiterea comenzii prin TCP\n");
						}

						fprintf(f, "%s", buffer);

					} else if (strcmp(command, "listsold\n") == 0 ||
							   strcmp(command, "transfer") == 0 ||
							   on_transfer == 1) {
						
						int send_check = send(sockfd_tcp, buffer, sizeof(buffer), 0);
						if (send_check == -1) {
							perror("Eroare la trimiterea comenzii prin TCP\n");
						}

						fprintf(f, "%s", buffer);
					
					} else if (strcmp(command, "unlock\n") == 0) {
						char unlock[BUFLEN];
						sprintf(unlock, "unlock %d", last_attempt);

						int send_check = sendto(sockfd_udp, unlock, sizeof(unlock), 0, (struct sockaddr *) &to_station, sizeof(struct sockaddr));
						if (send_check == -1) {
							perror("Eroare la trimiterea comenzii prin TCP\n");
						}

						fprintf(f, "%s", buffer);

					} else {
						int send_check = sendto(sockfd_udp, buffer, sizeof(buffer), 0, (struct sockaddr *) &to_station, sizeof(struct sockaddr));
						if (send_check == -1) {
							perror("Eroare la trimiterea comenzii prin UDP\n");
						}

						fprintf(f, "%s", buffer);
					}

					memcpy(last_command, buffer, sizeof(buffer));

				/* Cazul in care se primesc date prin socket-ul UDP */
				} else if (i == sockfd_udp) {
					memset(buffer, 0, BUFLEN);

					if ((n = recvfrom(sockfd_udp, buffer, BUFLEN, 0, (struct sockaddr *) &to_station, &addr_len)) <= 0) {
						if (n == 0) {
							printf("Serverul a fost inchis\n");

						} else {
							error("Eroare la primirea datelor prin UDP\n");
						}

						close(i);
						FD_CLR(i, &read_fds);
					
					} else {
						buffer[n] = '\n';
						fprintf(f, "%s\n", buffer);

						memcpy(last_command, buffer, sizeof(buffer));
						printf("%s\n", buffer);
					}

				/* Cazul in care primesc date prin socket-ul TCP */
				} else {
					memset(buffer, 0, BUFLEN);

					if ((n = recv(i, buffer, sizeof(buffer), 0)) <= 0) {
						if (n == 0) {
							printf("Serverul a fost inchis\n");

						} else {
							error("Eroare la primirea datelor prin TCP\n");
						}

						close(i);
						FD_CLR(i, &read_fds);
					
					} else {
						buffer[n] = '\n';
						fprintf(f, "%s\n", buffer);

						char copy[BUFLEN] = {0};
						memcpy(copy, buffer, sizeof(buffer));

						char *parser = strtok(copy, " ");
						parser = strtok(NULL, " ");
						
						if (strcmp(buffer, "quit") == 0) {
							close(sockfd_tcp);
	 						close(sockfd_udp);
							fclose(f);
   
     						return 0;

						} else if (strcmp(parser, "Welcome") == 0) {
							is_logged = 1;

						} else if (strcmp(buffer, "IBANK> Transfer realizat cu succes") == 0) {
							on_transfer = 0;
						}

						printf("%s\n", buffer);
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


