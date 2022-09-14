/*
 * client_1.c
 *
 *  Created on: Aug 9, 2022
 *      Author: tejbirsingh husainhanda
 */

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <signal.h>
#include <fcntl.h>
#define PORT 2005

int main(int argc, char *argv[]) {
	int getNumb(char*);
	char msg[255], buff[255];
	int server, portNumb, pid, n, i;
	char *CMD;
	char file_name[50];
	char **args = malloc(10 * sizeof(char*));
	struct sockaddr_in servAdd;     // server socket address
	char *codeResponse;
	int fd_1, fd_2;
	char buff_1[100];
	long int n1, counter,port;
	const char* add;
	if (argc != 3) {  // checking for the correct number of arguments while executing the client
		port = PORT;
		add = "127.0.0.1";
	}
	else{
		port = argv[2];
		add = argv[1];
	}
	if ((server = socket(AF_INET, SOCK_STREAM, 0)) < 0) {  //checking for socket creation
		fprintf(stderr, "Cannot create socket\n");
		exit(1);
	}

	servAdd.sin_family = AF_INET;
	sscanf(argv[2], "%d", &portNumb);
	servAdd.sin_port = htons(PORT);


	if (inet_pton(AF_INET, add, &servAdd.sin_addr) < 0) {
		fprintf(stderr, " inet_pton() has failed\n");
		exit(2);
	}

	if (connect(server, (struct sockaddr*) &servAdd, sizeof(servAdd)) < 0) {
		fprintf(stderr, "connect() has failed, exiting\n");
		exit(3);
	}

	read(server, msg, 255);
	fprintf(stderr, "%s\n", msg);

	pid = fork();

	if (pid)
		while (1) /* reading server's msgs */
			if ((n = read(server, msg, 255))) {
				msg[n] = '\0';
				fprintf(stderr, "%s\n", msg);
				codeResponse = strtok(msg, " ");
				if (!strcasecmp(codeResponse, "221")) {
					kill(pid, SIGTERM);
					close(server);
					exit(0);
				}
			}

	if (!pid) /* sending msgs to server */
		while (1) {
			if ((n = read(0, msg, 255))) {
				msg[n] = '\0';
				write(server, msg, strlen(msg) + 1);
				CMD = strtok(msg, " \n\0");
				i = 0;
				do {
					args[i++] = strtok(NULL, " \n\0");
				} while (args[i - 1] != NULL);
				i--;
				switch (getNumb(CMD)) {

				case 1: //"PORT"  it request ffrom the client to send 1 argument with it

					if (i != 1) {
						printf("Syntax error in parameters or arguments.");
						continue;
					}
					strcpy(file_name, "/tmp/");
					strcat(file_name, args[0]);
					break;

				case 2: //"RETR"  will send information and receive copy
					if (i != 1) {
						printf("Syntax error in parameters or arguments.");
						continue;
					}
					printf("file is %s done\n", file_name);
					if ((fd_1 = open(file_name, O_RDONLY)) == -1) {  //file name is assigned to fd_1
						perror("Client Error: Can't open file: ");
						continue;
					}
					if ((fd_2 = open(args[0], O_CREAT | O_WRONLY | O_TRUNC,  //file name is assigned to fd_2
							0700)) == -1) {
						perror("Client Error: Can't Create File: ");
						continue;
					}
					if (!fork()) {  //child process will perform the download
						printf("Client: Received File successfully ...\n");
						while ((n1 = read(fd_1, buff_1, 100)) > 0) {  //reading from the pipe
							if (write(fd_2, buff_1, n1) != n1) { // writing to file on client
								perror(
										"Client Error: won't be able to write to file: ");
								close(fd_1);
								close(fd_2);
								exit(0);
							}
						}
						if (n1 == -1) {
							perror(
									"Client Error: won't be able to Read form Pipe");
							close(fd_1);
							close(fd_2);
							exit(0);
						}
						close(fd_1);
						close(fd_2);
						printf("Client: File Saved.\n");
						exit(0);
					}
					close(fd_1);
					close(fd_2);
					break;

				case 3: // "APPE" or "STOR"
					// both the commands do the same function by sending the the file to the client
					if (i != 1) {
						printf(
								"Client Error: Syntax error in parameters or arguments.\n");
						continue;
					}
					if ((fd_1 = open(file_name, O_WRONLY)) == -1) {  //file name is assigned to fd_1
						printf(
								"Client Error: unable to create data connection.\n");
						continue;
					}
					if ((fd_2 = open(args[0], O_RDONLY)) == -1) {  //fd_2 is assigned to the file
						perror("Client Error: File is not available.");
						continue;
					}
					printf("Client: initiating file transfer.");
					if (!fork()) {  //reading the file and write to the filename where server will read
						while ((n1 = read(fd_2, buff_1, 100)) > 0) {

							if (write(fd_1, buff_1, n1) != n1) {
								perror(
										"Client gives Error: Can not be able write");
								exit(0);
							}
						}
						if (n1 == -1) {
							perror("Client gives Error: Can't be able to read");
							exit(0);
						}
						close(fd_1);
						close(fd_2);
						printf("Client: File Transfer completed.\n");
						exit(0);

						close(fd_1);
						close(fd_2);
					}
					break;

				case 4: //"QUIT"
					close(server);
					exit(0);
					break;

				}
			}

		}
}

int getNumb(char *CMD) {
	if (!strcasecmp(CMD, "PORT")) {
		return 1;
	} else if (!strcasecmp(CMD, "RETR")) {
		return 2;
	} else if (!strcasecmp(CMD, "STOR") || !strcasecmp(CMD, "APPE")) {
		return 3;
	} else if (!strcasecmp(CMD, "QUIT")) {
		return 4;
	} else
		return 0;
}

