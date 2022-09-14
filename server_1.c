/*
 * server_1.c
 *
 *  Created on: Aug 9, 2022
 *      Author: tejbirsingh husainhanda
 */

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <limits.h>
#define PORT 2005

void child(int);
int sockDispt, client, portNumb, status; // variables for socket connection
char fileOpen[50];
int log_in = 0,connectionCreated = 0;
void reply(int, char*);
int getCMDNumber(char *);
int checkLogin();
int checkConnection();
int pid;
void signal_handle(int signum){
	printf("Detected ^C or ^Z\n");
	unlink(fileOpen);
	log_in = 0;
	connectionCreated = 0;
	reply(sockDispt, "221 Service closing control connection. Logged out if appropriate.");
	close(sockDispt);
	exit(0);
}

pid_t processes[100];
int numProcesses;
int main(int argc, char *argv[]) {
	struct sockaddr_in server_addr; //  this is the client socket address structure

	if (argc > 1) {
		if( strcasecmp(argv[1],"-d")==0 && argv[2]!=NULL){ // server requires the argument to run, if '-d' present in arguments
			chdir(argv[2]); // changed the working directory
		}
		else{
			printf("Call  the model with port number: %s <Port Number>\n", argv[0]); // else exit
			exit(0);
		}
	}

	if ((sockDispt = socket(AF_INET, SOCK_STREAM, 0)) < 0) { //creating the socket and exiting if not able to
		fprintf(stderr, "socket cannot be created\n");
		exit(1);
	}
	sockDispt = socket(AF_INET, SOCK_STREAM, 0); //designating the server address family
	server_addr.sin_family = AF_INET;  //designating the server address family
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	//sscanf(argv[1], "%d", &portNumb);// taking the port no entered by the user

	server_addr.sin_port = htons(PORT);// designating the port no to connect on

	bind(sockDispt, (struct sockaddr*) &server_addr, sizeof(server_addr)); //binding th socket on the server address
	listen(sockDispt, 5);

	while (1) {  //  multiple clients will be accepted in infinite loop
		printf(" client connection is Ready to accept .. \n");
		client = accept(sockDispt, NULL, NULL);
		printf("Client successfully connected.\n");

		if (!fork()) // in this parent processes are fork to accept the next client
			child(client);  //child function will enter and perform child process for the client

		close(client);
		waitpid(0, &status, WNOHANG);
	}
}

void child(int sockDispt) {  // the function is created to handle all of the client requests, and will read commands from client
	signal(SIGINT, signal_handle);  // used to register cntrl c signal handler
	signal(SIGTSTP, signal_handle); // used to register cntrl z signal handler
	char message[255];
	int n, pid, i, Rename_from = 0; // to check rename command is working or not
	char *CMD;
	long int n1, cntr;
	char *tempfileOpen;
	char **args = malloc(10 * sizeof(char*));
	char renameFrom[PATH_MAX];
	int s = 0;
	int fd_1, fd_2;
	char buff[100];

	reply(sockDispt, "Connection successfully Established.Type help for List Of Commands");  // when the connection is successfully created
	while (1){    //all the command from client is accepted in this infinite loop
		if ((n = read(sockDispt, message, 255))) {

			message[n] = '\0';

			printf("Client Request the Command: %s\n", message);
			CMD = strtok(message, " \n\0");
			i = 0;
			do {
				args[i++] = strtok(NULL, " \n\0");
			} while (args[i - 1] != NULL);			i--;
			switch (getCMDNumber(CMD)) { // switch cases are used to read and compared all the commands
			case 1: // USER
				if(strcmp(args[0], "husain")==0){

				reply(sockDispt, "230 User logged in successfully , require pass.");
				}

				if(strcmp(args[0], "husain")!=0){
				log_in = 1;
				reply(sockDispt, "230 User not logged in successfully ");
				}
				break;

			case 2: // CWD  is used to change the working directory of the server
				if (!checkLogin()){
				reply (sockDispt,"user not log in");
				break;
				}
				if (i != 1) {
					reply(sockDispt, "501 Syntax error in parameters, failed.");  // if their is any syntax error during comparison
					continue;
				}
				if (chdir(args[0]) == 0)
					reply(sockDispt, "200 Working directory changed");  // working directory changed successfully
				else
					reply(sockDispt, "550 Requested action not taken.");  // error during run of the command
				break;

			case 3: // CDUP  to change the server directory to the parent directory
				if (!checkLogin()){
				reply (sockDispt,"user not log in");
			   break;
				}
				if (chdir("..") == 0)
					reply(sockDispt, "200 current Working directory has been changed");
				else
					reply(sockDispt, "550 Requested command action from client is not taken, failed.");
				break;

			case 4: // REIN   to reset the connection between client and server
				if (!checkLogin()){
				reply (sockDispt,"user not log in");
				break;
				}
				unlink(fileOpen);
				log_in = 0;
				connectionCreated = 0;
				printf( "connection reset, user logged out \n");
				break;

			case 5: // QUIT  // it will quit the server connection
				reply(sockDispt, "221 control connection is closed by the server. Logged out if appropriate.");
				close(sockDispt);
				exit(0);
				break;

			case 6:  //PORT  it will open FIFO to transfer file in a specific port between client and the server
				if (!checkLogin()){
				reply (sockDispt,"user not log in");
				break;
				}
				if (i != 1) {
					reply(sockDispt, "501 Syntax error in parameters, failed.");
					continue;
				}
				strcpy(fileOpen, "/tmp/");

				strcat(fileOpen, args[0]);
				unlink(fileOpen);
				if (mkfifo(fileOpen, 0777) != 0) {
					reply(sockDispt, "425 server is unable to open a data connection.");
					continue;
				}
				chmod("fileOpen", 0777);
				connectionCreated = 1;
				reply(sockDispt, "200 Command run.");
				break;
			case 7:  //RETR   to send the retrieve files to client
				if (!checkLogin() || !checkConnection()){
				reply (sockDispt,"user not log in");
				break;
				}
				if (i != 1) {
					reply(sockDispt, "501 Syntax error in parameters or arguments.");
					continue;
				}
				if ((fd_1 = open(fileOpen, O_WRONLY)) == -1) {  //File descriptor write into FIFO from where client will read
					reply(sockDispt, "425 server is unable to open a data connection.");
					continue;
				}
				if ((fd_2 = open(args[0], O_RDONLY)) == -1) {  //  read the FD file name with args[0]
					reply(sockDispt,"550 Requested command action from client is not taken. File unavailable.");
					continue;
				}
				reply(sockDispt,"125 initiating the file transfer.");
				processes[numProcesses++] = fork();  // to transfer the child processes
				if (!processes[numProcesses-1]) {
					while ((n1 = read(fd_2, buff, 100)) > 0) {  // reading bytes form the buffer file

						if (write(fd_1, buff, n1) != n1) {  // writing the 100  bytes in buff
							reply(sockDispt,"552 Requested command file action is aborted. won't be able to write");
							exit(0);
						}
					}
					if (n1 == -1) {
						reply(sockDispt,"552 Requested command file action is aborted. won't be able to read");
						exit(0);
					}
					close(fd_1);  // close the FD for child processes
					close(fd_2);
					reply(sockDispt, "250 Requested file action okay, completed.");
					exit(0);
				}
				close(fd_1);
				close(fd_2);
				break;

			case 8: // "STOR"  accept the data to store the data as a file in server site
				if (!checkLogin() || !checkConnection()){
				reply (sockDispt,"user not log in");
				break;
				}
				if (i != 1) { // checking for arguments
					reply(sockDispt, "501 Syntax error in parameters or arguments.");
					continue;
				}
				if ((fd_1 = open(fileOpen, O_RDONLY)) == -1) {
					reply(sockDispt, "425 server is unable to open a data connection.");
					continue;
				}
				if ((fd_2 = open(args[0], O_CREAT | O_WRONLY | O_TRUNC, 0700)) == -1) {
					reply(sockDispt,"550 Requested command action from client is not taken. won't be able to create file.");
					continue;
				}
				processes[numProcesses++] = fork(); //creating a fork process for file transfer
				if (!processes[numProcesses-1]) {
					reply(sockDispt,"125 initiating the file transfer.");
					while ((n1 = read(fd_1, buff, 100)) > 0) {
						if (write(fd_2, buff, n1) != n1) {
							reply(sockDispt, "552 Requested command file action is aborted. won't be able to read.");
							close(fd_1);
							close(fd_2);
							exit(0);
						}
					}
					if (n1 == -1) { // check if file was transfered
						reply(sockDispt,"Requested command file action is aborted. won't be able to read from pipe.");
						close(fd_1);
						close(fd_2);
						exit(0);
					}
					close(fd_1);
					close(fd_2);
					reply(sockDispt, "250 Requested file action okay, completed.");
					exit(0);
				}
				close(fd_1);
				close(fd_2);
				break;

			case 9: //"APPE"  append the data in the store file
				if (!checkLogin() || !checkConnection()){
				reply (sockDispt,"user not log in");
				break;
				}

				long int n1;
				if (i != 1) {
					reply(sockDispt, "501 Syntax error in parameters or arguments.");
					continue;
				}
				if ((fd_1 = open(fileOpen, O_RDONLY)) == -1) {
					reply(sockDispt, "425 server is unable to open a data connection.");
					continue;
				}
				if ((fd_2 = open(args[0], O_CREAT | O_WRONLY | O_APPEND, 0700)) == -1) {
					reply(sockDispt,"550 Requested command action from client is not taken. won't be able to create file.");
					continue;
				}
				processes[numProcesses++] = fork();
				if (!processes[numProcesses-1]) {
					reply(sockDispt,"125 initiating the file transfer.");
					while ((n1 = read(fd_1, buff, 100)) > 0) {
						if (write(fd_2, buff, n1) != n1) {
							reply(sockDispt,"552 Requested command file action is aborted. won't be able to write.");
							close(fd_1);
							close(fd_2);
							exit(0);
						}
					}
					if (n1 == -1) {
						reply(sockDispt,"552 Requested command file action is aborted. won't be able to read.");
						close(fd_1);
						close(fd_2);
						exit(0);
					}
					close(fd_1);
					close(fd_2);
					reply(sockDispt, "250 Requested file action okay, completed.");
					exit(0);
				}
				close(fd_1);
				close(fd_2);
				break;

			case 10: // "REST"
				reply(sockDispt, "250 Requested file action okay, completed.");
				break;
			case 11: //"RNFR"  used to rename the selected file taken from client
				if (!checkLogin()){
				reply (sockDispt,"user not log in");
				break;
			    }
				if (i != 1) {
					reply(sockDispt, "501 Syntax error in parameters or arguments.");
					continue;
				}
				strcpy(renameFrom, args[0]);// copies the file name from user
				Rename_from = 1;
				reply(sockDispt, "200 command okay.");
				break;

			case 12: //"RNTO"  rename to which name we have to give to the file
				if (!checkLogin()){
				reply (sockDispt,"user not log in");
				break;
			    }
				if (i != 1) {
					reply(sockDispt, "501 Syntax error in parameters or arguments.");
					continue;
				}
				if (!Rename_from) {  // to check if RNFR is executed or not
					reply(sockDispt, "503 Bad sequence of Commands.");
					continue;
				}
				if (rename(renameFrom, args[0]) == 0)
					reply(sockDispt, "250 Requested file action okay, completed.");
				else
					reply(sockDispt, "553 Requested action not taken; File name not allowed.");
				break;
			case 13: // "ABOR"  // to abort all the commands

				reply(sockDispt, "250 Requested file action okay, completed.");
				break;
			case 14: // "DELE"  to delete the file
				if (!checkLogin()){
				reply (sockDispt,"user not log in");
				break;
				}
				if (i != 1) {
					reply(sockDispt, "501 Syntax error in parameters or arguments.");
					continue;
				}
				if (remove(args[0]) == 0)
					reply(sockDispt, "250 Requested file action okay, completed.");
				else
					reply(sockDispt, "550 Requested action not taken.");
				break;
			case 15: // "RMD" used to remove the directory))
				if (!checkLogin()){
				reply (sockDispt,"user not log in");
				break;
				}
				if (i != 1) {
					reply(sockDispt, "501 Syntax error in parameters or arguments.");
					continue;
				}
				if (rmdir(args[0]) == 0)
					reply(sockDispt, "250 Requested file action okay, completed.");
				else
					reply(sockDispt, "550 Requested action not taken.");
				break;
			case 16: // "MKD"))   to make new directory
				if (!checkLogin()){
				reply (sockDispt,"user not log in");
				break;
				}
				if (i != 1) {
					reply(sockDispt, "501 Syntax error in parameters or arguments.");
					continue;
				}
				if (mkdir(args[0], 0700) == 0)
					reply(sockDispt, "250 Requested file action okay, completed.");
				else
					reply(sockDispt, "550 Requested action not taken.");
				break;
			case 17: // "PWD" current working directory
			{
				char *buf = malloc(PATH_MAX);
				reply(sockDispt, getcwd(buf, PATH_MAX));
				break;
			}
			case 18: // "LIST" list all the contents of the directory
				if (!checkLogin() || !checkConnection()){
			    reply (sockDispt,"user not log in");
				break;
				}
				reply(sockDispt,"150 File status okay; about to open data connection.\n");

				reply(sockDispt,"125 initiating the file transfer.\n");
				DIR *d;
				struct dirent *dir;
				d = opendir(".");
				if (d) {
					while ((dir = readdir(d)) != NULL) {
						reply(sockDispt, dir->d_name);
						reply(sockDispt, "\n");
					}
					closedir(d);
				}
				reply(sockDispt, "250 Requested file action okay, completed.");
				break;
			case 19: //"STAT"  to give the pid's of all the current working processes
				if (!checkLogin()){
				reply (sockDispt,"user not log in");
				break;
				}
				reply(sockDispt,"List of processes (pid's) in progress -> \n");
				for(int i = 0; i<numProcesses ; i++){
					if(!kill(processes[i], 0)){
						char temp_1[10];
						sprintf(temp_1, "%d\n", processes[i]);
						reply(sockDispt, temp_1);
					}
				}
				reply(sockDispt,"200 Command Okay.");
				break;

			case 20: // "NOOP" sends an ok reply
				reply(sockDispt, "200 Command okay.");
				break;
			case 21: // PASS to check password
				if(strcmp(args[0], "tejbir")==0){
				log_in = 1;
				reply(sockDispt, "230 User logged in successfully ");
				}

				if(strcmp(args[0], "tejbir")!=0){
				log_in = 1;
				reply(sockDispt, "Incoreect Password ");
				}
				break;
				break;
			case 22:reply(sockDispt, "Welcom to Tejbir And Husain's Server Here Are all the commands\n1.User: for login takes argument user<argument> try user husain \n2.Pass: for authentication takes argument pass<argument> try pass tejbir\n3:CDUP  to change the server directory to the parent directory\n4.REIN:to reset the connection between client and server\n5.QUIT: it will quit the server connection\n6.PORT:it will open FIFO to transfer file in a specific port between client and the server\n7.RETR: to send the retrieve files to client\n8.APPE:append the data in the store file\n9.REST:request file\n10.RNFR:used to rename the selected file taken from client \n11.RNTO:rename to which name we have to give to the file\n12.ABOR: to abort all the commands\n13.DELE: to delete the file \n14:RMD used to remove the directory \n15.MKD :to make new directory\n 16.PWD :current working directory \n17.LIST:list all the contents of the directory \n18.STAT: to give the pid's of all the current working processes \n19.NOOP: sends an ok reply.\n20.STOR :accept the data to store the data as a file in server site");
				break;
			default:
				reply(sockDispt, "502 Command is not implemented.");
			}
		}}
}

	void reply(int fd, char *message) {  //to write the message in socket from where client will read and process it
		write(fd, message, strlen(message));
	}

	int getCMDNumber(char *CMD) {  //function is created to call the each case for each command
		if (!strcasecmp(CMD, "USER")) {
			return 1;
		} else if (!strcasecmp(CMD, "CWD")) {
			return 2;
		} else if (!strcasecmp(CMD, "CDUP")) {
			return 3;
		} else if (!strcasecmp(CMD, "REIN")) {
			return 4;
		} else if (!strcasecmp(CMD, "QUIT")) {
			return 5;
		} else if (!strcasecmp(CMD, "PORT")) {
			return 6;
		} else if (!strcasecmp(CMD, "RETR")) {
			return 7;
		} else if (!strcasecmp(CMD, "STOR")) {
			return 8;
		} else if (!strcasecmp(CMD, "APPE")) {
			return 9;
		} else if (!strcasecmp(CMD, "REST")) {
			return 10;
		} else if (!strcasecmp(CMD, "RNFR")) {
			return 11;

		} else if (!strcasecmp(CMD, "RNTO")) {
			return 12;
		} else if (!strcasecmp(CMD, "ABOR")) {
			return 13;
		} else if (!strcasecmp(CMD, "DELE")) {
			return 14;
		} else if (!strcasecmp(CMD, "RMD")) {
			return 15;
		} else if (!strcasecmp(CMD, "MKD")) {
			return 16;
		} else if (!strcasecmp(CMD, "PWD")) {
			return 17;
		} else if (!strcasecmp(CMD, "LIST")) {
			return 18;
		} else if (!strcasecmp(CMD, "STAT")) {
			return 19;
		} else if (!strcasecmp(CMD, "NOOP")) {
			return 20;
			}
		else if (!strcasecmp(CMD, "PASS")) {
			return 21;
		}
		else if (!strcasecmp(CMD, "HELP")) {
			return 22;
		}
		else {
			return 0;
		}
	}

	int checkLogin()  // method is created to check if the user is logged in or not
	{
		if (!log_in)
		{
			reply(sockDispt,"530 user is not successfully logged in");
					return 0;
		}

		return 1;
	}
	int checkConnection()  // method is created to check if the data connection is created or not
	{
		if (!connectionCreated)
		{
			reply(sockDispt,"425 server is not able to create connection");
			return 0;
		}
		return 1;
	}
