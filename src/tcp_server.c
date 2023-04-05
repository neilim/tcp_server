#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h>

#include "tcp_server.h"

int tcp_server(server_t*server){
	for(int i=0;i<server->max_clients;i++){
		server->clients[i].fd=0;
	}

	if((server->master_socket=socket(AF_INET,SOCK_STREAM,0))==0){
#ifdef DEBUG
		printf("%s: socket creating failed\n",server->name);
#endif
		exit(EXIT_FAILURE);
	}

	if(setsockopt(server->master_socket,SOL_SOCKET,SO_REUSEADDR,&(int){1},sizeof(int))<0){
#ifdef DEBUG
		printf("%s: setsockopt failed\n",server->name);
#endif
		exit(EXIT_FAILURE);
	}

	server->address.sin_family= AF_INET;
	server->address.sin_addr.s_addr= INADDR_ANY;
	server->address.sin_port=htons(server->port);

	if(bind(server->master_socket,(struct sockaddr*)&server->address,sizeof(server->address))<0){
#ifdef DEBUG
		printf("%s: bind failed\n",server->name);
#endif
		exit(EXIT_FAILURE);
	}
#ifdef DEBUG
	printf("%s: listening on port:%d\n",server->name,server->port);
#endif

	if(listen(server->master_socket,server->max_pending_connections)<0){
#ifdef DEBUG
		printf("%s: listening failed\n",server->name);
#endif
		exit(EXIT_FAILURE);
	}

	server->addrlen=sizeof(server->address);
#ifdef DEBUG
	printf("%s: waiting for connections\n",server->name);
#endif

	while(1){
		FD_ZERO(&server->readfds);

		FD_SET(server->master_socket,&server->readfds);
		server->max_sd=server->master_socket;

		for(int i=0;i<server->max_clients;i++){
			server->sd=server->clients[i].fd;

			if(server->sd>0)
				FD_SET(server->sd,&server->readfds);

			if(server->sd>server->max_sd)
				server->max_sd=server->sd;
		}

		server->activity=select(server->max_sd+1,&server->readfds,NULL,NULL,NULL);

		if((server->activity<0)&&(errno!=EINTR)){
#ifdef DEBUG
			printf("%s: select error\n",server->name);
#endif
		}
		if(FD_ISSET(server->master_socket,&server->readfds)){
			if((server->new_socket=accept(server->master_socket,(struct sockaddr*)&server->address,(socklen_t*)&server->addrlen))<0){
#ifdef DEBUG
				printf("%s: accept error\n",server->name);
#endif
				exit(EXIT_FAILURE);
			}
#ifdef DEBUG
			printf("%s: new connection registered:fd:%d,%s:%d\n",server->name,server->new_socket,inet_ntoa(server->address.sin_addr),ntohs(server->address.sin_port));
#endif

			if(server->welcome_message!=NULL){
				if(send(server->new_socket,server->welcome_message,strlen(server->welcome_message),0)!=strlen(server->welcome_message)){
#ifdef DEBUG
					printf("%s: send failed\n",server->name);
#endif
				}
#ifdef DEBUG
				printf("%s: welcome message sent successfully\n",server->name);
#endif
			}
			for(int i=0;i<server->max_clients;i++){
				if(server->clients[i].fd==0){
					server->clients[i].fd=server->new_socket;
					getpeername(server->sd,(struct sockaddr*)&server->address,(socklen_t*)&server->addrlen);
					strcpy(server->clients[i].ip,inet_ntoa(server->address.sin_addr));
					server->clients[i].port=ntohs(server->address.sin_port);
#ifdef DEBUG
					printf("%s: adding to list of sockets as %d\n",server->name,i);
#endif
					break;
				}
			}
		}

		for(int i=0;i<server->max_clients;i++){
			server->sd=server->clients[i].fd;
			if(FD_ISSET(server->sd,&server->readfds)){
				if((server->valread=read(server->sd,server->buffer,server->buffer_size-1))==0){

#ifdef DEBUG
					printf("%s: host disconnected:ip:%s:%d\n",server->name,server->clients[i].ip,server->clients[i].port);
#endif
					close(server->sd);
					server->clients[i].fd=0;
					bzero(server->clients[i].ip,(size_t)(IP_LENGTH));
					server->clients[i].port=0;
				}
				else{
#ifdef DEBUG
					printf("%s: host:%s:%d sent:%s",server->name,server->clients[i].ip,server->clients[i].port,server->buffer);
#endif
					server->buffer[server->valread]='\0';
					server->tcp_answerer(&server->clients[i],server->buffer,strlen(server->buffer));
				}
			}
		}
	}
}

bool tcp_server_config(server_t*server,char*name,uint16_t port,int max_pending_connections,int max_clients,int buffer_size){
	server->name=(char*)calloc(strlen(name),sizeof(char));
	strcpy(server->name,name);
	server->welcome_message=NULL;
	server->max_pending_connections=max_pending_connections;
	server->max_clients=max_clients;
	server->clients=(client_t*)calloc(server->max_clients,sizeof(client_t));
	server->buffer_size=buffer_size;
	server->buffer=(char*)calloc(server->buffer_size,sizeof(char));
	server->port=port;
	return true;
}

bool tcp_server_set_welcome_message(server_t*server,char*message){
	char*new_message;
	if(message==NULL){
		if(server->welcome_message==NULL){
#ifdef DEBUG
			printf("%s: welcome message deactivated\r\n",server->name);
#endif
			return true;
		}
		free(server->welcome_message);
		server->welcome_message=NULL;
#ifdef DEBUG
		printf("%s: welcome message deactivated\r\n",server->name);
#endif
		return true;
	}

	if(server->welcome_message==NULL){
		server->welcome_message=(char*)calloc(strlen(message),sizeof(char));
	}
	else{
		new_message=(char*)realloc(server->welcome_message,strlen(message));
		server->welcome_message=new_message;
	}
	strcpy(server->welcome_message,message);

	if(server->welcome_message==NULL){
#ifdef DEBUG
		printf("%s: welcome message setting error\r\n",server->name);
#endif
		return false;
	}
	else{
#ifdef DEBUG
		printf("%s: welcome message set to: %s",server->name,server->welcome_message);
#endif
		return true;
	}
}

bool tcp_server_set_answerer(server_t*server,tcp_answerer_t answerer){
	if(answerer==NULL){
		return false;
	}
	else{
		server->tcp_answerer=answerer;
		return true;
	}
}

bool tcp_server_send_to_client(server_t *server,client_t*client,char*message,int length){
	send(client->fd,message,length,0);
	return true;
}

int tcp_server_template_answerer(client_t*client,char*buffer,int len){
	return send(client->fd,buffer,len,0);
}
