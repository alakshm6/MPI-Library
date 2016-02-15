#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include<string.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include<time.h>
/* Define a port and set each process's port relative to this constant */
#define PORT 12345
#define MAX_CLIENTS 15
#define MPI_Comm double
#define MPI_COMM_WORLD 1
#define MPI_DOUBLE 1
#define MPI_CHAR 2
#define MPI_Status double
#define MPI_Datatype int
/* Function declarations */
void get_hostnames(char*);
void hostnames_to_ip();
void MPI_Init(int*,char***);
void MPI_Recv(void*,double,MPI_Datatype,int,int,MPI_Comm,MPI_Status*);
void MPI_Send(void*,double,MPI_Datatype,int,int,MPI_Comm);
void MPI_Comm_size(MPI_Comm,int*);
void MPI_Comm_rank(MPI_Comm,int*);
double MPI_Wtime();
void MPI_Finalize();
/* Global variables */
// Hostnames of all the nodes
char hostnames[100][100];
char ip[100][100];
// Number of processors, rank and server socket fd
int numproc,rank,mysockfd;
struct sockaddr_in my_server,another_server;
clock_t initial_clock;
/*int main(int argc,char** argv){
	printf("TEST 1\n");
	get_hostnames(argv[1]);
	printf("TEST 2\n");
	int i = 0;
	char* host = hostnames[i];
	while(i< 9 && host != NULL){
		printf("%s\n",host);
		host = hostnames[i];
		i++;
	}
	printf("TEST 3\n");
	hostnames_to_ip();
	for(i=0;i<8;i++)
		printf("%s\n",ip[i]);
}*/
/* Store host names of all nodes in the global scope */
void get_hostnames(char* hosts){
        char* token;
	int i = 0;
	token =  strtok(hosts,",");
	//strcpy(hostnames[i],token);
	//i++;
	while(token != NULL){
		//printf("%s\n\n\n",token);
		strcpy(hostnames[i],token);
		//printf("%s\n",hostnames[i]);
		i++;
		token = strtok(NULL,",");
	}
	free(token);
}
/* Convert hostnames to corresponding IP Addresses */
void hostnames_to_ip(){
	int i=0,j;
	char* hostname;
	struct hostent* host_details;
	struct in_addr **addr_list;
	
	//printf("TEST 40\n");
	hostname=hostnames[i];
	//printf("TEST 4\n");
	
	while(i <8 && hostname != NULL){
		
		hostname=hostnames[i];
		if ((host_details = gethostbyname(hostname)) == NULL) {  // get the host info
			printf("Not a valid hostname");
		}
		
		addr_list = (struct in_addr **)host_details->h_addr_list;
		for(j = 0; addr_list[j] != NULL; j++) {
			strcpy(ip[i],inet_ntoa(*addr_list[j]));
			break;
		}
		i++;
	}
}
void MPI_Init(int* argc,char*** argv){
	char** temp = *argv;
	//printf("Arg1 : %s",temp[1]);
	//printf("Arg2 : %d\n",atoi(temp[2]));
	//printf("Arg3 : %d\n",atoi(temp[3]));
	// Get hostnames for all the nodes passed as the argument
	get_hostnames(temp[1]);
	//hostnames_to_ip();
	// Get the total number of processes
	numproc = atoi(temp[2]);
	// Get the rank of the current process
	rank = atoi(temp[3]);
	// Open a server socket for this process
	mysockfd = socket(AF_INET,SOCK_STREAM,0);
	if(mysockfd < 0) {
		printf("Error creating socket");
		exit(1);
	}
	bzero((char*) &my_server, sizeof(my_server));
	// Set the server socket with its corresponding address and port
	my_server.sin_family = AF_INET;
	my_server.sin_addr.s_addr = INADDR_ANY;
	my_server.sin_port = htons(PORT + rank);
	// Bind socket to port and address
	if(bind(mysockfd,(struct sockaddr*) &my_server,sizeof(my_server)) < 0){
		printf("Error binding socket");
		exit(1);
	}
	// Listen to incoming connections and set maximum connections to number of processes
	listen(mysockfd,numproc);
	initial_clock = clock();
}

// Set size of the current system i.e number of processes 
void MPI_Comm_size(MPI_Comm comm,int* num){
	//printf("numproc in comm size : %d\n",numproc);
	*num = numproc;
	//printf("num in comm size : %d\n",*num);
}
// Set rank of the current process
void MPI_Comm_rank(MPI_Comm comm,int* r){
	//printf("rank in comm rank : %d\n",rank);
	*r= rank;
	//printf("rank in comm rank : %d\n",*r);
}
// Return time in microseconds
double MPI_Wtime(){
	 struct timeval curr_time;
         gettimeofday (&curr_time, NULL);
        return (double)((curr_time.tv_sec*1e6 + curr_time.tv_usec));

}
// Receive messages from a source
void MPI_Recv(void* buf,double count,MPI_Datatype type,int src_rank,int tag,MPI_Comm comm,MPI_Status* status ){
	int client_sock_fd;
	double n;
	socklen_t clilen;
	struct sockaddr_in client;
	int retries = 0;
	clilen = sizeof(client);
	//printf("TEST : %d ABOUT TO RECEIVE FROM %d SOCKET \n",rank,src_rank);
	// Try accepting connections until there is some client who wants to send.
	while(1){
		//if(retries < 10)
		//	printf("Waiting in rank %d to receive packets from %d \n ",rank,src_rank);
		client_sock_fd = accept(mysockfd, (struct sockaddr *) &client, &clilen);
		if(client_sock_fd < 0){
			//retries++;
			continue;
		}
		break;
	}
	//printf("TEST : %d RECEIVED FROM %d SOCKET \n",rank,src_rank);
	// Set size of receive buffer based on incoming message datatype
	long size = (type==MPI_DOUBLE)?sizeof(double)*count:sizeof(char)*count;
	//n = read(client_sock_fd,buf,size);  
	// Receive message from source
	// Wait until the complete message is received
	n = recv(client_sock_fd,buf,size,MSG_WAITALL);
    if (n < 0) printf("ERROR reading from socket\n");
	if(n != size) printf("Message not read fully : Read %lf instead of %lf\n",n,size);
	close(client_sock_fd);
}
// Send messages to destination node
void MPI_Send(void* buf,double count,MPI_Datatype type,int dest_rank,int tag,MPI_Comm comm ){
	struct sockaddr_in server;
	int server_socket_fd;
	double n;
	int retries = 0;
	struct hostent *server_host;
	// Get server hostname from global hostnames array
	server_host = gethostbyname(hostnames[dest_rank]);
	// Create socket for the communication
	server_socket_fd = socket(AF_INET,SOCK_STREAM,0);
	// Set up socket for communication with server
	server.sin_family = AF_INET;
	bcopy((char *)server_host->h_addr, 
         (char *)&server.sin_addr.s_addr,
         server_host->h_length);
	//server.sin_addr.s_addr = inet_addr(ip[dest_rank]);
	server.sin_port = htons(PORT + dest_rank);
	//printf("TEST : %d ABOUT TO CONNECT TO SERVER SOCKET %d\n",rank,dest_rank);
	// Retry connecting until the server is up and ready to accept new connections
	while(1){
		//if(retries < 10)
		//	printf("Trying to connect from rank %d to %d\n",rank,dest_rank);
		if (connect(server_socket_fd,(struct sockaddr *) &server,sizeof(server)) ==  0)
			break;
		//retries++;
	}
	//printf("TEST : %d  CONNECTED  TO SERVER SOCKET %d\n",rank,dest_rank);
	// Set size of receive buffer based on incoming message datatype
	long size = (type==MPI_DOUBLE)?sizeof(double)*count:sizeof(char)*count;
	// Send messages to destination
	n = write(server_socket_fd,buf,size);  
        if (n < 0){ 
		printf("ERROR writing to socket on rank %d\n",rank);
		exit(1);
	}
	if(n != size) printf("Message not written fully : Written %lf instead of %ld\n",n,size);
	close(server_socket_fd);
}
// Close server socket for reusing ports
void MPI_Finalize(){
    //free(hostnames);
    //free(ip);
    close(mysockfd);    
}

