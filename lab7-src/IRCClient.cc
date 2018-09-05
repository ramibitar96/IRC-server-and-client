
#include <stdio.h>
#include <gtk/gtk.h>
#include <time.h>
#include <curses.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#define MAX_MESSAGES 100
#define MAX_MESSAGE_LEN 300
#define MAX_RESPONSE (20 * 1024)

GtkListStore * list_rooms;
GtkListStore * list_users;
GtkWidget * list;
GtkWidget * list2;
GtkWidget * messages;
GtkWidget * myMessage;
GtkWidget * room_box;
GtkWidget * username_box;
GtkWidget * password_box;
GtkWidget * room_dialog;
GtkWidget * account_dialog;
GtkWidget * table;
GtkWidget * window;
GtkWidget * scrolled_window;
GtkWidget * tree_view;
GtkCellRenderer * cell;
GtkTreeViewColumn * column;
GtkWidget * scrolled_window2;
GtkWidget * tree_view2;
GtkCellRenderer * cell2;
GtkTreeViewColumn * column2;

const char * defaulthost = "moore20.cs.purdue.edu";
int defaultport = 32096;
char * pastRoom;
char * curRoom;
const char * username;
const char * password; 

int open_client_socket(const char * host, int port) {
	// Initialize socket address structure
	struct  sockaddr_in socketAddress;
	
	// Clear sockaddr structure
	memset((char *)&socketAddress,0,sizeof(socketAddress));
	
	// Set family to Internet 
	socketAddress.sin_family = AF_INET;
	
	// Set port
	socketAddress.sin_port = htons((u_short)port);
	
	// Get host table entry for this host
	struct  hostent  *ptrh = gethostbyname(host);
	if ( ptrh == NULL ) {
		perror("gethostbyname");
		exit(1);
	}
	
	// Copy the host ip address to socket address structure
	memcpy(&socketAddress.sin_addr, ptrh->h_addr, ptrh->h_length);
	
	// Get TCP transport protocol entry
	struct  protoent *ptrp = getprotobyname("tcp");
	if ( ptrp == NULL ) {
		perror("getprotobyname");
		exit(1);
	}
	
	// Create a tcp socket
	int sock = socket(PF_INET, SOCK_STREAM, ptrp->p_proto);
	if (sock < 0) {
		perror("socket");
		exit(1);
	}
	
	// Connect the socket to the specified server
	if (connect(sock, (struct sockaddr *)&socketAddress,
		    sizeof(socketAddress)) < 0) {
		perror("connect");
		exit(1);
	}
	
	return sock;
}

int sendCommand(const char * host, int port, const char * command, const char * user,
		const char * password, char * args, char * response) {
	int sock = open_client_socket( host, port);

	write(sock, command, strlen(command));
	write(sock, " ", 1);
	write(sock, user, strlen(user));
	write(sock, " ", 1);
	write(sock, password, strlen(password));
	write(sock, " ", 1);
	if (args != NULL) 
	write(sock, args, strlen(args));
	write(sock, "\r\n",2);

	int n = 0;
	int len = 0;
	while ((n=read(sock, response+len, MAX_RESPONSE - len))>0) {
		len += n;
	}
	close(sock);
}

void update_list_rooms() {
    if (username != NULL && password != NULL) {
    	gtk_list_store_clear(list_rooms);
    	GtkTreeIter iter;
    	char * response = (char *) malloc(sizeof(char) * MAX_RESPONSE);
    	sendCommand(defaulthost, defaultport, "LIST-ROOMS", username, password, NULL, response);
    	char * input = strtok(response, "\r\n");
    	while (input != NULL) {
    		gchar *msg = g_strdup_printf (input);
    	    gtk_list_store_append (GTK_LIST_STORE (list_rooms), &iter);
       		gtk_list_store_set (GTK_LIST_STORE (list_rooms), &iter, 0, msg, -1);
			g_free (msg);
 			input = strtok(NULL, "\r\n");   	
    	}
    }
}

void update_list_users() {
    if (username != NULL && password != NULL && curRoom != NULL) {
    	gtk_list_store_clear(list_users
    		);
    	GtkTreeIter iter;
    	char * response = (char *) malloc(sizeof(char) * MAX_RESPONSE);
    	sendCommand(defaulthost, defaultport, "GET-USERS-IN-ROOM", username, password, curRoom, response);
    	char * input = strtok(response, "\r\n");
    	while (input != NULL) {
    		gchar *msg = g_strdup_printf (input);
    	    gtk_list_store_append (GTK_LIST_STORE (list_users), &iter);
       		gtk_list_store_set (GTK_LIST_STORE (list_users), &iter, 0, msg, -1);
			g_free (msg);
 			input = strtok(NULL, "\r\n");   	
    	}
    }

}

void room_ok_button_clicked() {
	char * response;
	int temp;
	char * newRoomName;
	const char * command = "CREATE-ROOM";;
	if (username != NULL && password != NULL) {
		newRoomName = strdup((char *) gtk_entry_get_text((GtkEntry *)room_box));
		if(strcmp(newRoomName, "Room Name") != 0) {
			temp = sendCommand(defaulthost, defaultport, command, username, password, newRoomName, response);
		}
		gtk_widget_destroy (room_dialog);	
	}	
	gtk_widget_destroy (room_dialog);

}


void room_button_clicked() {
	room_dialog = gtk_dialog_new_with_buttons("Create a Room", NULL, GTK_DIALOG_MODAL, NULL);
	GtkWidget *room_area = gtk_dialog_get_content_area((GtkDialog *) room_dialog);
	GtkWidget *ok_button = gtk_button_new_with_label ("OK");

	room_box = gtk_entry_new();
	gtk_box_pack_start(GTK_BOX(room_area), room_box, TRUE,TRUE,0);
	gtk_entry_set_text((GtkEntry *)room_box, "Room Name");
	gtk_widget_show(room_box);

	gtk_box_pack_end(GTK_BOX(room_area), ok_button, TRUE, TRUE, 0);
	gtk_widget_show(ok_button);

	g_signal_connect(G_OBJECT(ok_button), "clicked", G_CALLBACK(room_ok_button_clicked), NULL);
	gtk_widget_show(room_dialog);
}

void send_button_clicked() {
	if (username != NULL && password != NULL) {
		int temp;
		char * response;
		char * message;
		char * finalMessage;
		message = (char *) gtk_entry_get_text((GtkEntry *)myMessage); 
		if(curRoom != NULL) {
			if (strcmp(message, "") != 0) {
			temp = sprintf(finalMessage,"%s %s", curRoom, message);
			sendCommand(defaulthost, defaultport, "SEND-MESSAGE", username, password, finalMessage, response);
			gtk_entry_set_text((GtkEntry *)myMessage, "");
			}
		}
		return;
	}
}

void account_ok_button_clicked() {
	if (username == NULL && password == NULL) {
		int temp;
		char * response;
		username = strdup((const char *)gtk_entry_get_text((GtkEntry *)username_box));
		password = strdup((const char *)gtk_entry_get_text((GtkEntry *)password_box));
		sendCommand(defaulthost, defaultport, "ADD-USER", username, password, NULL, response);
	}	
	gtk_widget_destroy (account_dialog);
}

void account_button_clicked() {
	account_dialog = gtk_dialog_new_with_buttons("Create Account", NULL, GTK_DIALOG_MODAL, NULL);
	GtkWidget *account_area = gtk_dialog_get_content_area((GtkDialog *) account_dialog);
	GtkWidget *ok_button = gtk_button_new_with_label ("OK");

	username_box = gtk_entry_new();
	gtk_box_pack_start(GTK_BOX(account_area), username_box, TRUE,TRUE,0);
	gtk_entry_set_text((GtkEntry *)username_box, "Username");
	gtk_widget_show(username_box);

	password_box = gtk_entry_new();
	gtk_box_pack_start(GTK_BOX(account_area), password_box, TRUE,TRUE, 0);
	gtk_entry_set_text((GtkEntry *)password_box, "Password");
	gtk_widget_show(password_box);

	gtk_box_pack_end(GTK_BOX(account_area), ok_button, TRUE, TRUE, 0);
	gtk_widget_show(ok_button);

	g_signal_connect(G_OBJECT(ok_button), "clicked", G_CALLBACK(account_ok_button_clicked), NULL);
	gtk_widget_show(account_dialog);

}

/* Create the list of "messages" */
static GtkWidget *create_list( const char * titleColumn, GtkListStore *model )
{
    int i;
   
    /* Create a new scrolled window, with scrollbars only if needed */
    scrolled_window = gtk_scrolled_window_new (NULL, NULL);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
				    GTK_POLICY_AUTOMATIC, 
				    GTK_POLICY_AUTOMATIC);
   
    tree_view = gtk_tree_view_new ();
    gtk_container_add (GTK_CONTAINER (scrolled_window), tree_view);
    gtk_tree_view_set_model (GTK_TREE_VIEW (tree_view), GTK_TREE_MODEL (model));
    gtk_widget_show (tree_view);
   
    cell = gtk_cell_renderer_text_new ();

    column = gtk_tree_view_column_new_with_attributes (titleColumn,
                                                       cell,
                                                       "text", 0,
                                                       NULL);
  
    gtk_tree_view_append_column (GTK_TREE_VIEW (tree_view),
	  		         GTK_TREE_VIEW_COLUMN (column));

    return scrolled_window;
}

static GtkWidget *create_list2( const char * titleColumn, GtkListStore *model )
{

    int i;
   
    /* Create a new scrolled window, with scrollbars only if needed */
    scrolled_window2 = gtk_scrolled_window_new (NULL, NULL);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window2),
				    GTK_POLICY_AUTOMATIC, 
				    GTK_POLICY_AUTOMATIC);
   
    tree_view2 = gtk_tree_view_new ();
    gtk_container_add (GTK_CONTAINER (scrolled_window2), tree_view2);
    gtk_tree_view_set_model (GTK_TREE_VIEW (tree_view2), GTK_TREE_MODEL (model));
    gtk_widget_show (tree_view2);
   
    cell2 = gtk_cell_renderer_text_new ();

    column2 = gtk_tree_view_column_new_with_attributes (titleColumn,
                                                       cell2,
                                                       "text", 0,
                                                       NULL);
  
    gtk_tree_view_append_column (GTK_TREE_VIEW (tree_view2),
	  		         GTK_TREE_VIEW_COLUMN (column2));

    return scrolled_window2;
}

static void insert_text( GtkTextBuffer *buffer, const char * initialText )
{
   GtkTextIter iter;
 
   gtk_text_buffer_get_iter_at_offset (buffer, &iter, 0);
   gtk_text_buffer_insert (buffer, &iter, initialText,-1);
}

   
static GtkWidget *create_text( const char * initialText )
{
   GtkWidget *scrolled_window;
   GtkWidget *view;
   GtkTextBuffer *buffer;

   view = gtk_text_view_new ();
   buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (view));

   scrolled_window = gtk_scrolled_window_new (NULL, NULL);
   gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
		   	           GTK_POLICY_AUTOMATIC,
				   GTK_POLICY_AUTOMATIC);

   gtk_container_add (GTK_CONTAINER (scrolled_window), view);
   insert_text (buffer, initialText);

   gtk_widget_show_all (scrolled_window);

   return scrolled_window;
}

void update_messages() {
    if (username != NULL && password != NULL && curRoom != NULL) {
    	char * response = (char *) malloc(sizeof(char) * MAX_RESPONSE);
     	char * tempMessage = (char *) malloc(sizeof(char) * MAX_RESPONSE);;
    	int temp = sprintf(tempMessage, "0 %s", curRoom); 
		sendCommand(defaulthost, defaultport, "GET-MESSAGES", username, password, tempMessage, response);
		messages = create_text(response);
		gtk_table_attach_defaults (GTK_TABLE (table), messages, 0, 5, 3, 6);
    	gtk_widget_show (messages);

    }
}

void enter_room() {
	int temp;
	char * finalMessage1 = (char *) malloc(sizeof(char) * MAX_MESSAGE_LEN);
	char * finalMessage2 = (char *) malloc(sizeof(char) * MAX_MESSAGE_LEN);
	pastRoom = curRoom;
	char * response;
	GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree_view));
	GtkTreeIter iter; 
	GtkTreeModel * model = gtk_tree_view_get_model(GTK_TREE_VIEW(tree_view));
	bool r = gtk_tree_selection_get_selected(selection, &model, &iter); 
	if(r) {
		gtk_tree_model_get(model, &iter, 0, &curRoom, -1);
		temp = sprintf(finalMessage1,"%s entered the room", curRoom);
		sendCommand(defaulthost, defaultport,"ENTER-ROOM", username, password, curRoom,response);
		sendCommand(defaulthost, defaultport, "SEND-MESSAGE", username, password, finalMessage1, response);

		if (pastRoom != NULL) {
			temp = sprintf(finalMessage2,"%s left the room", pastRoom);
			sendCommand(defaulthost, defaultport, "SEND-MESSAGE", username, password, finalMessage2, response);
			sendCommand(defaulthost, defaultport,"LEAVE-ROOM", username, password, pastRoom,response);
		}
	}
	free(finalMessage1);
	free(finalMessage2);
}

static gboolean
time_handler(GtkWidget *widget)
{
  if (widget->window == NULL) 
  	return FALSE;
  update_list_rooms();
  update_list_users();
  update_messages();
  return TRUE;
}



int main(int argc, char *argv[] )
{

    gtk_init (&argc, &argv);
   	curRoom = (char *) malloc(sizeof(char) * 30);
   	curRoom = NULL;
    window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title (GTK_WINDOW (window), "IRC Client");
    g_signal_connect (window, "destroy",
	              G_CALLBACK (gtk_main_quit), NULL);
    gtk_container_set_border_width (GTK_CONTAINER (window), 10);
    gtk_widget_set_size_request (GTK_WIDGET (window), 450, 400);

    table = gtk_table_new (7, 5, TRUE);
    gtk_container_add (GTK_CONTAINER (window), table);
    gtk_table_set_row_spacings(GTK_TABLE (table), 6);
    gtk_table_set_col_spacings(GTK_TABLE (table), 6);
    gtk_widget_show (table);

    list_rooms = gtk_list_store_new (1, G_TYPE_STRING);
   	update_list_rooms();
    list = create_list ("Rooms", list_rooms);
    gtk_table_attach_defaults (GTK_TABLE (table), list, 0, 2, 0, 3);
    gtk_widget_show (list);

    list_users = gtk_list_store_new (1, G_TYPE_STRING);
    update_list_users();
    list2 = create_list2 ("Users In Room", list_users);
    gtk_table_attach_defaults (GTK_TABLE (table), list2, 3, 5, 0, 3);
    gtk_widget_show (list2);
   
   	messages = create_text("");
 	gtk_table_attach_defaults (GTK_TABLE (table), messages, 0, 5, 3, 6);
    gtk_widget_show (messages);

 	myMessage = gtk_entry_new();
  	gtk_table_attach_defaults (GTK_TABLE (table), myMessage, 0, 5, 6, 7);
    gtk_widget_show (myMessage);

    GtkWidget *send_button = gtk_button_new_with_label ("Send");
    gtk_table_attach_defaults(GTK_TABLE (table), send_button, 0, 1, 7, 8); 
    gtk_widget_show (send_button);

    GtkWidget *account_button = gtk_button_new_with_label ("Create Account");
    gtk_table_attach_defaults(GTK_TABLE (table), account_button, 4, 5, 7, 8); 
    gtk_widget_show (account_button);

    GtkWidget *room_button = gtk_button_new_with_label ("Create Room");
    gtk_table_attach_defaults(GTK_TABLE (table), room_button, 2, 3, 7, 8); 
    gtk_widget_show (room_button);

    g_signal_connect(tree_view, "row-activated", G_CALLBACK(enter_room),NULL);
    g_signal_connect(G_OBJECT(send_button), "clicked", G_CALLBACK(send_button_clicked), NULL);
    g_signal_connect(G_OBJECT(account_button), "clicked", G_CALLBACK(account_button_clicked), NULL);
    g_signal_connect(G_OBJECT(room_button), "clicked", G_CALLBACK(room_button_clicked), NULL);

	g_timeout_add(5000,(GSourceFunc) time_handler, (gpointer) window);
    
    gtk_widget_show (table);
    gtk_widget_show (window);

    gtk_main ();
    

    return 0;
}