
#ifndef IRC_SERVER
#define IRC_SERVER

#define PASSWORD_FILE "password.txt"

struct userNode {
	char * userName;
	char * password; 
	struct userNode * next;
};

typedef struct userNode userNode;

struct userList {
	userNode * head;
};

typedef struct userList userList;

void userList_add(userList * list, char * value);
void userList_remove(userList * list, int value);

struct messageNode {
	char * message;
	struct messageNode * next;
};

typedef struct messageNode messageNode;

struct messageList {
	messageNode * head;
};

typedef struct messageList messageList;

void message_insert_last(messageList * list, char * value);
void message_remove_ith(messageList * list, int ith);

struct roomNode {
	int messages;
	char * name;
	struct roomNode * next;
	userList * roomUserList;
	messageList * messagesList;
};


typedef struct roomNode roomNode;

struct roomList {
	roomNode * head;
};

typedef struct roomList roomList;

void roomList_add(roomList * list, char * value);

class IRCServer {
	// Add any variables you need
	FILE * pass;
	userList *users;
	roomList *rooms;
private:
	int open_server_socket(int port);

public:
	void initialize();
	bool checkPassword(int fd, const char * user, const char * password);
	void processRequest( int socket );
	void addUser(int fd, const char * user, const char * password, const char * args);
	void enterRoom(int fd, const char * user, const char * password, const char * args);
	void leaveRoom(int fd, const char * user, const char * password, const char * args);
	void sendMessage(int fd, const char * user, const char * password, const char * args);
	void getMessages(int fd, const char * user, const char * password, const char * args);
	void getUsersInRoom(int fd, const char * user, const char * password, const char * args);
	void getAllUsers(int fd, const char * user, const char * password, const char * args);
	void runServer(int port);
	void createRoom(int fd, const char * user, const char * password, const char * args);
	void listRooms(int fd, const char * user, const char * password, const char * args); 
};

#endif
