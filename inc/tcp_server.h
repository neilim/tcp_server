#ifndef TCP_SERVER_H
#define TCP_SERVER_H

#define DEBUG

#include <stdbool.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define IP_LENGTH 16

typedef struct {
	int fd;
	char ip[IP_LENGTH];
	uint16_t port;
} client_t;

typedef int (*tcp_answerer_t)(client_t*client,char*buffer,int len);

typedef struct {
	char*name;
	int master_socket;
	uint16_t port;
	int addrlen;
	int new_socket;
	int activity;
	int valread;
	int sd;
	int max_sd;
	int max_pending_connections;
	int max_clients;
	int buffer_size;
	struct sockaddr_in address;
	char*buffer;
	fd_set readfds;
	char*welcome_message;
	client_t*clients;
	tcp_answerer_t tcp_answerer;
} server_t;

/**
 * @brief	This function will initialize the tcp server and it contains
 * 			infinite loop for handling connections and call the answerer function
 * 			when new message received.
 *
 * @param	server:	The server itself.
 *
 * @return	TBD
 */
int tcp_server(server_t*server);

/**
 * @brief	You can configure the tcp server with this function before start
 * 			it with tcp_server().
 *
 * @param	server: The server itself
 * @param	name:	This is the human readable name of the server
 * @param	port:	This is the port, where the server will listen
 * @param	max_pending_connections:	Defines how many pending connections allowed
 * @param	max_clients:	Defines how many clients can connect to server
 * @param	buffer_site:	Maximum allowed incoming packet size in bytes
 *
 * @return	true when success else false
 */
bool tcp_server_config(server_t*server,char*name,uint16_t port,int max_pending_connections,int max_clients,int buffer_size);

/**
 * @brief	You can set the welcome message with this function.
 * 			This message will be send to the new client when the connection success.
 *
 * @param	server:	The server itself.
 * @param	message:	The welcome message.
 *
 * @return true when success else false
 */
bool tcp_server_set_welcome_message(server_t*server,char*message);

/**
 * @brief	You can set the answerer function with this function.
 *
 * @param	server:	The server itself
 * @param	answerer:	The answerer function. Please read the tcp_answerer_t
 * 						definitions brief.
 *
 * @return	true when success else false
 */
bool tcp_server_set_answerer(server_t*server,tcp_answerer_t answerer);

/**
 * @brief TBD
 */
bool tcp_server_send_to_client(server_t *server,client_t*client,char*message,int length);

/**
 * @brief TBD
 */
bool tcp_server_drop_client(server_t *server,client_t *client);

/**
 * @brief TBD
 */
bool tcp_server_client_to_ban_list(server_t *server,client_t *client);

/**
 * @brief TBD
 */
bool tcp_server_remove_client_from_ban_list(server_t *server,client_t *client);

/**
 * @brief	This function is a template for an answerer.
 *
 * @param	client:	The answerer can access all of client parameters.
 * @param	buffer:	The message what the client sent.
 * @param	len:	Length of incoming message.
 *
 * @return	Length of sent message in bytes.
 */
int tcp_server_template_answerer(client_t*client,char*buffer,int len);

#endif	//TCP_SERVER_H
