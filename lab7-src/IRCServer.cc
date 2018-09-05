
const char * usage =
"                                                               \n"
"IRCServer:                                                   \n"
"                                                               \n"
"Simple server program used to communicate multiple users       \n"
"                                                               \n"
"To use it in one window type:                                  \n"
"                                                               \n"
"   IRCServer <port>                                          \n"
"                                                               \n"
"Where 1024 < port < 65536.                                     \n"
"                                                               \n"
"In another window type:                                        \n"
"                                                               \n"
"   telnet <host> <port>                                        \n"
"                                                               \n"
"where <host> is the name of the machine where talk-server      \n"
"is running. <port> is the port number you used when you run    \n"
"daytime-server.                                                \n"
"                                                               \n";

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

#include "IRCServer.h"

int QueueLength = 5;
int compare(const void * s1, const void * s2);

int
IRCServer::open_server_socket(int port) {

	// Set the IP address and port for this server
	struct sockaddr_in serverIPAddress; 
	memset( &serverIPAddress, 0, sizeof(serverIPAddress) );
	serverIPAddress.sin_family = AF_INET;
	serverIPAddress.sin_addr.s_addr = INADDR_ANY;
	serverIPAddress.sin_port = htons((u_short) port);
  
	// Allocate a socket
	int masterSocket =  socket(PF_INET, SOCK_STREAM, 0);
	if ( masterSocket < 0) {
		perror("socket");
		exit( -1 );
	}

	// Set socket options to reuse port. Otherwise we will
	// have to wait about 2 minutes before reusing the sae port number
	int optval = 1; 
	int err = setsockopt(masterSocket, SOL_SOCKET, SO_REUSEADDR, 
			     (char *) &optval, sizeof( int ) );
	
	// Bind the socket to the IP address and port
	int error = bind( masterSocket,
			  (struct sockaddr *)&serverIPAddress,
			  sizeof(serverIPAddress) );
	if ( error ) {
		perror("bind");
		exit( -1 );
	}
	
	// Put socket in listening mode and set the 
	// size of the queue of unprocessed connections
	error = listen( masterSocket, QueueLength);
	if ( error ) {
		perror("listen");
		exit( -1 );
	}

	return masterSocket;
}

void
IRCServer::runServer(int port)
{
	int masterSocket = open_server_socket(port);

	initialize();
	
	while ( 1 ) {
		
		// Accept incoming connections
		struct sockaddr_in clientIPAddress;
		int alen = sizeof( clientIPAddress );
		int slaveSocket = accept( masterSocket,
					  (struct sockaddr *)&clientIPAddress,
					  (socklen_t*)&alen);
		
		if ( slaveSocket < 0 ) {
			perror( "accept" );
			exit( -1 );
		}
		
		// Process request.
		processRequest( slaveSocket );		
	}
}

int
main( int argc, char ** argv )
{
	// Print usage if not enough arguments
	if ( argc < 2 ) {
		fprintf( stderr, "%s", usage );
		exit( -1 );
	}
	
	// Get the port from the arguments
	int port = atoi( argv[1] );

	IRCServer ircServer;

	// It will never return
	ircServer.runServer(port);
	
}

//
// Commands:
//   Commands are started y the client.
//
//   Request: ADD-USER <USER> <PASSWD>\r\n
//   Answer: OK\r\n or DENIED\r\n
//
//   REQUEST: GET-ALL-USERS <USER> <PASSWD>\r\n
//   Answer: USER1\r\n
//            USER2\r\n
//            ...
//            \r\n
//
//   REQUEST: CREATE-ROOM <USER> <PASSWD> <ROOM>\r\n
//   Answer: OK\n or DENIED\r\n
//
//   Request: LIST-ROOMS <USER> <PASSWD>\r\n
//   Answer: room1\r\n
//           room2\r\n
//           ...
//           \r\n
//
//   Request: ENTER-ROOM <USER> <PASSWD> <ROOM>\r\n
//   Answer: OK\n or DENIED\r\n
//
//   Request: LEAVE-ROOM <USER> <PASSWD>\r\n
//   Answer: OK\n or DENIED\r\n
//
//   Request: SEND-MESSAGE <USER> <PASSWD> <MESSAGE> <ROOM>\n
//   Answer: OK\n or DENIED\n
//
//   Request: GET-MESSAGES <USER> <PASSWD> <LAST-MESSAGE-NUM> <ROOM>\r\n
//   Answer: MSGNUM1 USER1 MESSAGE1\r\n
//           MSGNUM2 USER2 MESSAGE2\r\n
//           MSGNUM3 USER2 MESSAGE2\r\n
//           ...\r\n
//           \r\n
//
//    REQUEST: GET-USERS-IN-ROOM <USER> <PASSWD> <ROOM>\r\n
//    Answer: USER1\r\n
//            USER2\r\n
//            ...
//            \r\n
//

void
IRCServer::processRequest( int fd )
{
	// Buffer used to store the comand received from the client
	const int MaxCommandLine = 1024;
	char commandLine[ MaxCommandLine + 1 ];
	int commandLineLength = 0;
	int n;
	
	// Currently character read
	unsigned char prevChar = 0;
	unsigned char newChar = 0;
	
	//
	// The client should send COMMAND-LINE\n
	// Read the name of the client character by character until a
	// \n is found.
	//

	// Read character by character until a \n is found or the command string is full.
	while ( commandLineLength < MaxCommandLine &&
		read( fd, &newChar, 1) > 0 ) {

		if (newChar == '\n' && prevChar == '\r') {
			break;
		}
		
		commandLine[ commandLineLength ] = newChar;
		commandLineLength++;

		prevChar = newChar;
	}
	
	// Add null character at the end of the string
	// Eliminate last \r
	commandLineLength--;
        commandLine[ commandLineLength ] = 0;
    char *save_ptr1;
	printf("RECEIVED: %s\n", commandLine);
	char * command = strtok_r(commandLine, " ", &save_ptr1);
	char * user = strtok_r(NULL, " ", &save_ptr1);
	char * password = strtok_r(NULL, " ", &save_ptr1);;
	char * args = strtok_r(NULL, "", &save_ptr1);;


	printf("%s\n",commandLine);
	printf("command=%s\n", command);
	printf("user=%s\n", user);
	printf( "password=%s\n", password );
	printf("args=%s\n", args);

	if (!strcmp(command, "ADD-USER")) {
		addUser(fd, user, password, args);
	}
	else if (!strcmp(command, "ENTER-ROOM")) {
		enterRoom(fd, user, password, args);
	}
	else if (!strcmp(command, "LEAVE-ROOM")) {
		leaveRoom(fd, user, password, args);
	}
	else if (!strcmp(command, "SEND-MESSAGE")) {
		sendMessage(fd, user, password, args);
	}
	else if (!strcmp(command, "GET-MESSAGES")) {
		getMessages(fd, user, password, args);
	}
	else if (!strcmp(command, "GET-USERS-IN-ROOM")) {
		getUsersInRoom(fd, user, password, args);
	}
	else if (!strcmp(command, "GET-ALL-USERS")) {
		getAllUsers(fd, user, password, args);
	}
	else if (!strcmp(command, "CREATE-ROOM")) {
		createRoom(fd, user, password, args);
	}
	else if (!strcmp(command, "LIST-ROOMS")) {
		listRooms(fd, user, password, args);
	} else {
		const char * msg =  "UNKNOWN COMMAND\r\n";
		write(fd, msg, strlen(msg));
	}

	//Send OK answer
	//const char * msg =  "OK\n";
	//write(fd, msg, strlen(msg));

	close(fd);	
}

void userList_add(userList * list, char * value, char * value2) {
	userNode * n = (userNode *) malloc(sizeof(userNode));
	n->userName = strdup(value);
	n->password = strdup(value2);
	n->next = list->head;
	list->head = n;
}

void roomList_add(roomList * list, char * value) {
	roomNode * n = (roomNode *) malloc(sizeof(roomNode));
	n->messages = 0;
	n->name = value;
	n->next = list->head;
	list->head = n;
	n->roomUserList = (userList *) malloc(sizeof(userList));
	n->messagesList = (messageList *) malloc(sizeof(messageList));
	n->roomUserList->head = NULL;
	n->messagesList->head = NULL;
}

void message_remove_ith(messageList * list, int ith) {
	if (list->head != NULL) {
		messageNode * temp;
		messageNode *prev;
        	temp = list->head; 
		int i;
		for (i = 0; i <= ith; i++) {
			if (ith - i == 0) {
				if (ith == 0) {
					temp = temp->next;
					free(list->head);
					list->head = temp;				
					return; 	
				}			
				prev->next = temp->next;
				if(temp != NULL)
				free(temp);
				temp=NULL;
				return;
			}
			if (temp->next != NULL) {
				prev = temp;
				temp = temp->next;
			} else 
				return;
		}
	}
}

void message_insert_last(messageList * list, char * value) {
	messageNode *temp1;
    messageNode *temp2;  
    temp1 = (messageNode *) malloc(sizeof(messageNode));
    temp1->message=value;  
  	temp2 = list->head;  
  
    if(list->head == NULL)  {  
		temp1->next = NULL;
		list->head = temp1;
		return;
    }  
    else  
    {    
       while(temp2->next != NULL)  
       temp2 = temp2->next;
       
       temp2->next=temp1;    
       temp1->next=NULL;  
       return;
    }  
}

void userList_remove(userList * list, int value) {
	if (list->head != NULL) {
		userNode * temp;
		userNode *prev;
        temp = list->head; 
		int i;
		for (i = 0; i <= value; i++) {
			if (value - i == 0) {
				if (value == 0) {
					temp = temp->next;
					free(list->head);
					list->head = temp;				
					return; 	
				}			
				prev->next = temp->next;
				if(temp != NULL)
					free(temp);
				temp=NULL;
				return;
			}
			if (temp->next != NULL) {
				prev = temp;
				temp = temp->next;
			} else 
				return;
		}
	}
}

void
IRCServer::initialize()
{
	users = (userList *) malloc(sizeof(userList));
	users->head = NULL;
	pass = fopen(PASSWORD_FILE, "a+");
	int max = 10000;
	char * line = (char *) malloc(max*sizeof(char *));
	char * userLine = (char *) malloc(sizeof(char *));
	char * passLine = (char *) malloc(sizeof(char *));
	int i;
	int j;
	while (fgets(line, max, pass) != NULL) {
		i = 0;
		j = 0;
			while (*line != ' ') {
				*userLine = *line;
				line++;
				userLine++;
				i++;
			}
			line++;
			*userLine = '\0';
			while (*line != '\n') {
				*passLine = *line;
				line++;
				passLine++;
				j++;
			}
			*passLine = '\0';
			userList_add(users, userLine - i, passLine - j);
	}
	fclose(pass);
	rooms = (roomList *) malloc(sizeof(roomList));
	rooms->head = NULL;
}

bool
IRCServer::checkPassword(int fd, const char * user, const char * password) {
	userNode * n = users->head;
	char * a = strdup(user);
	char * b = strdup(password);
	while(n != NULL) {
		if(!strcmp(n->userName, a) && !strcmp(n->password, b)) {
			return true;
		}
		n = n->next;
	}
	return false;
}

void 
IRCServer::createRoom(int fd, const char * user, const char * password, const char * args) {
		if(checkPassword(fd, user, password)) {
			char * a = strdup(args);
			roomNode * r = rooms->head;
			while (r != NULL) {
				if (strcmp(r->name, args) == 0) {
					const char * msg =  "ERROR (Room already exists)\r\n";
					write(fd, msg, strlen(msg));
					return;
				}
				r = r->next;
			}
			roomList_add(rooms, a);
			const char * msg =  "OK\r\n";
			write(fd, msg, strlen(msg));
			return;		
		}
		const char * msg =  "ERROR (Wrong password)\r\n";
		write(fd, msg, strlen(msg));
		return;
}

void
IRCServer::addUser(int fd, const char * user, const char * password, const char * args)
{
	pass = fopen(PASSWORD_FILE, "a+");
	userNode * n = users->head;
	if (users->head != NULL) {
		while (n != NULL) {
			if(strcmp(user,n->userName) == 0) {
					const char * msg =  "DENIED\r\n";
					write(fd, msg, strlen(msg));
					return;	
			}
			n= n->next;
		}	
	}
	char * a = strdup(user);
	char * b = strdup(password);
	fprintf(pass, "%s %s\n", a,b);
	userList_add(users, a, b);
	fclose(pass);
	const char * msg =  "OK\r\n";
	write(fd, msg, strlen(msg));
	return;		
}
void IRCServer::listRooms(int fd, const char * user, const char * password, const char * args) 
{
	int temp;
	if (checkPassword(fd, user, password)) {
		if (rooms->head != NULL) {
			roomNode * r = rooms->head;
			int i = 0;
			while (r != NULL) {
				i++;
				r = r->next;
			}
			r = rooms->head;
			char * allRooms[i];
			int j;
			for (j = 0; j < i; j++) {
				allRooms[j] = r->name;
					r = r->next;
				}
				qsort(allRooms, i, sizeof(char *), compare);
				char * msg = (char *) malloc(sizeof(char) * 20);
				int k;
				for (k = 0; k < i; k++) {
					temp = sprintf(msg, "%s\r\n", allRooms[k]);
					write(fd, msg, strlen(msg));			
				}
				return;
		} 
		const char * msg =  "ERROR (No room)\r\n";
		write(fd, msg, strlen(msg));
		return;			
	}
	const char * msg =  "ERROR (Wrong password)\r\n";
	write(fd, msg, strlen(msg));
	return;	
}

void
IRCServer::enterRoom(int fd, const char * user, const char * password, const char * args)
{
	if(checkPassword(fd, user, password)) {
		if (rooms->head != NULL) {
			roomNode * r = rooms->head;
			while (r != NULL) {
				if (strcmp(r->name,args) == 0) {
					userNode * u = r->roomUserList->head;
					char * a = strdup(user);
					char * b = strdup(password);
					while (u != NULL) {
						if (strcmp(a, u->userName) == 0) {
							const char * msg =  "OK\r\n";
							write(fd, msg, strlen(msg));
							return;
						}
						u = u->next;
					}
					userList_add(r->roomUserList, a, b);
					const char * msg =  "OK\r\n";
					write(fd, msg, strlen(msg));
					return;	
				}
				r = r->next;
			}
		}
		const char * msg =  "ERROR (No room)\r\n";
		write(fd, msg, strlen(msg));
		return;		
	}
	const char * msg =  "ERROR (Wrong password)\r\n";
	write(fd, msg, strlen(msg));
	return;		
}


void
IRCServer::leaveRoom(int fd, const char * user, const char * password, const char * args)
{
	char * a = strdup(user);
	if(checkPassword(fd, user, password)) {
		if (rooms->head != NULL) {
			roomNode * r = rooms->head;
			while (r != NULL) {
				if(strcmp(r->name, args) == 0) {
					userNode * n = r->roomUserList->head;
					int i = 0;
					while(n != NULL) {
						if(strcmp(n->userName, a) == 0) {
							userList_remove(r->roomUserList,i);
							const char * msg =  "OK\r\n";
							write(fd, msg, strlen(msg));
							return;	
						}
						i++;
						n = n->next;
					}
					const char * msg =  "ERROR (No user in room)\r\n";
					write(fd, msg, strlen(msg));
					return;		
				}
				r = r->next;
			}
			const char * msg =  "ERROR (No room)\r\n";
			write(fd, msg, strlen(msg));
			return;	
		}
	}
	const char * msg =  "ERROR (Wrong password)\r\n";
	write(fd, msg, strlen(msg));
	return;	
}

void
IRCServer::sendMessage(int fd, const char * user, const char * password, const char * args)
{
	if(checkPassword(fd, user, password)) {
		char * a = strdup(args);
		char * name = strdup(user);
		char * room;
		char * message;
		char * finished = (char *) malloc(sizeof(char) * 1024);
		int temp;
		if (a != NULL) {
			room = strsep(&a, " ");
			message = strsep(&a, "");
		}
		roomNode * r = rooms->head;
		while(r != NULL) {
			if (strcmp(r->name, room) == 0) {
				userNode * u = r->roomUserList->head;
				while (u != NULL) {
					if(strcmp(u->userName, name) == 0) {
						temp = sprintf(finished ,"%s %s\r\n",name, message);
						if (r->messages < 100) {
							message_insert_last(r->messagesList, finished);
							r->messages++;
							const char * msg =  "OK\r\n";
							write(fd, msg, strlen(msg));
							return;		
						} else {
							message_remove_ith(r->messagesList, 0);
							message_insert_last(r->messagesList, finished);
							const char * msg =  "OK\r\n";
							write(fd, msg, strlen(msg));
							return;		
						}
					}
					u = u->next;
				}
				const char * msg =  "ERROR (user not in room)\r\n";
				write(fd, msg, strlen(msg));
				return;		
			}
			r = r->next;
		}
		const char * msg =  "ERROR (No room with that name)\r\n";
		write(fd, msg, strlen(msg));
		return;	

	}
	const char * msg =  "ERROR (Wrong password)\r\n";
	write(fd, msg, strlen(msg));
	return;	
}

void
IRCServer::getMessages(int fd, const char * user, const char * password, const char * args)
{
	if(checkPassword(fd, user, password)) {
		char * msg1 = (char *) malloc(sizeof(char) * 1024);
		int i = 0;
		char * a = strdup(args);
		char * messageNumber;
		char * room;
		char * name = strdup(user);
		if (a != NULL) {
			messageNumber = strsep(&a, " ");
			room = strsep(&a, "");
			int messageNum = atoi(messageNumber);
			roomNode * r = rooms->head;
			while (r != NULL) {
				if(strcmp(room, r->name) == 0) {
					userNode * u = r->roomUserList->head;
					while (u != NULL) {
						if (strcmp(user,u->userName) == 0) {	
							if (messageNum < r->messages) {							
								messageNode * m = r->messagesList->head;
								if (messageNum != 0) {
									for (i = 0; i < messageNum; i++) {
											if (m->next != NULL) {
												m = m->next;
											} 
									}
								}
								int temp;
								while (m != NULL) {
									temp = sprintf(msg1, "%d %s", i, m->message);
									write(fd, msg1, strlen(msg1));
									m = m->next;	
									i++;
								}
								const char * msg =  "\r\n";
								write(fd, msg, strlen(msg));
								return;	
							}
							const char * msg =  "NO-NEW-MESSAGES\r\n";
							write(fd, msg, strlen(msg));
							return;	
						}
						u = u->next;
					}
					const char * msg =  "ERROR (User not in room)\r\n";
					write(fd, msg, strlen(msg));
					return;	
				}
				r = r->next;
			}
			return;
		}
	}
	const char * msg =  "ERROR (Wrong password)\r\n";
	write(fd, msg, strlen(msg));
	return;	
}

void
IRCServer::getUsersInRoom(int fd, const char * user, const char * password, const char * args)
{
	if(checkPassword(fd, user, password)) {
		int temp;
		roomNode * r = rooms->head;
		while(r != NULL) {
			if (strcmp(r->name, args) == 0) {
				if (r->roomUserList->head != NULL) {
					int i = 0;
					userNode * u = r->roomUserList->head;
					while (u != NULL) {
						i++;
						u = u->next;
					}
					if (i == 0) {
						const char * msg2 =  "ERROR (No users in room)\r\n";
						write(fd, msg2, strlen(msg2));
						return;	
					}
					char * allUserInRoom[i];
					int j;
					u = r->roomUserList->head;
					for (j = 0; j < i; j++) {
						allUserInRoom[j] = u->userName;
						u = u->next;
					}
					qsort(allUserInRoom, i, sizeof(char *), compare);
					char * msg = (char *) malloc(sizeof(char) * 1024);
					int k;
					for (k = 0; k < i; k++) {
						temp = sprintf(msg, "%s\r\n", allUserInRoom[k]);
						write(fd, msg, strlen(msg));			
					}
					const char * msg2 =  "\r\n";
					write(fd, msg2, strlen(msg2));
					return;	
				}
			}
			r = r->next;
		}
		const char * msg2 =  "\r\n";
		write(fd, msg2, strlen(msg2));	
		return;	
	}
	const char * msg =  "ERROR (Wrong password)\r\n";
	write(fd, msg, strlen(msg));
	return;	
}

void
IRCServer::getAllUsers(int fd, const char * user, const char * password,const  char * args)
{
	int temp;
	if(checkPassword(fd, user, password)) {
		if (users->head != NULL) {
				int i = 0;
				userNode * n = users->head;
				while(n != NULL) {
					i++;
					n = n->next;
				}
				char * allUsers[i];
				int j;
				n = users->head;
				for (j = 0; j < i; j++) {
					allUsers[j] = n->userName;
					n = n->next;
				}
				qsort(allUsers, i, sizeof(char *), compare);
				char * msg = (char *) malloc(sizeof(char) * 1024);
				int k;
				for (k = 0; k < i; k++) {
					temp = sprintf(msg, "%s\r\n", allUsers[k]);
					write(fd, msg, strlen(msg));			
				}
				write(fd, "\r\n",strlen("\r\n"));
				return;
		}	
		const char * msg =  "ERROR (No users)\r\n";
		write(fd, msg, strlen(msg));
		return;	
	}
	const char * msg =  "ERROR (Wrong password)\r\n";
	write(fd, msg, strlen(msg));
	return;	
}

int compare(const void * s1, const void * s2) {
	const char *a = *(const char**)s1;
    const char *b = *(const char**)s2;
    return strcmp(a,b);
}
