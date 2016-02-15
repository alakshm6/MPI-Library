#include "mympi.h"
/* closes all the server socket connections at the end */
int MPI_Finalize(){
        close(sd);
        return 0;
}
/* returns number of tasks */
int MPI_Comm_size(int comm, int *mysize){
        *mysize=size;
}
/* creates client socket and sends the message to  server using server ip address and port number */
int MPI_Send(const void *buf, int count, int datatype, int dest, int tag,int comm){
    int sockfd, portno, n;

    struct sockaddr_in serv_addr; /* structure to hold server's address */
    struct hostent *server;       /* pointer to a host table entry */

    char buffer[2048*2048*2];
    double buffer2[10];
    portno = 35657+dest;
	 /* Create a socket */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);  
    if (sockfd < 0)
        error("ERROR opening socket");
	/* get host ip address*/
    server = gethostbyname(addr_map[dest]);  
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }
    bzero((char *) &serv_addr, sizeof(serv_addr));   /* clear sockaddr structure   */
    serv_addr.sin_family = AF_INET;		/* set family to Internet     */
    bcopy((char *)server->h_addr,(char *)&serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_port = htons(portno);
    while(1){
                if (connect(sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) >= 0){
        //              fprintf(stderr,"connect success");
                        break;
                }
        }
    if(datatype==1){
         memcpy(buffer, buf,sizeof(char)* count);
         n = write(sockfd,buffer,count);
    }
    else {
         //fprintf(stderr,"written to socket %f ", *(double*)buf);
         memcpy(buffer2,(double*)buf,sizeof(double)*count);
         n = write(sockfd,buf,count);
        // fprintf(stderr,"written to socket %f ",buffer2);
     }
  // fprintf(stderr,"written to socket %f ",buffer2);
    if (n < 0)
         error("ERROR writing to socket");
    bzero(buffer,256);
    close (sockfd);
    return 0;
}
/* revieves messages from the socket by accepting connections */
int MPI_Recv(void *buf, int count, int datatype, int source, int tag, int comm, int *status){
        struct sockaddr_in cad; /* struct to hold client address */
        int alen = sizeof(cad);
        //char message_to_archive[2048*2048*2];
      //  double message_to_archive2[10];
        int sd2,n;
        //fprintf(stderr,"SERVER: Waiting for contact ...%d\n",rank);
        while (1) {
            if((sd2=accept(sd, (struct sockaddr *)&cad, &alen)) >= 0) {
            //  fprintf(stderr, "accept success\n");
                        break;
            }
        }

//     n = recv (sd2,buf, count, MSG_WAITALL);
        if(datatype == 1){
               // bzero(message_to_archive,2048*2048*2);
                n = recv (sd2, (char*)buf,count , MSG_WAITALL);
              //  memcpy(buf , message_to_archive, sizeof(char)*count);
        }
        else {
               // bzero(message_to_archive2, 10);
                n = recv (sd2,(double*)buf,count , MSG_WAITALL);
                //memcpy((double*)buf , message_to_archive2, sizeof(double)*count);
              //  fprintf(stderr,"read from socket %8.8f ",buf);
        }
        //      fprintf(stderr,"read from socket %f ",buf);
        close(sd2);
}
/* returns rank */
int MPI_Comm_rank(int comm, int *myrank){
        *myrank = rank;
}
/* returns wall time*/
double MPI_Wtime(){
        struct timeval tv;
        double curtime;
        gettimeofday(&tv, NULL);
        //curtime=tv.tv_sec;
        curtime=tv.tv_sec+(tv.tv_usec/1000000.0);
//      printf(" \n curtime= %10.10f",curtime);
        return curtime;
}


/* Parses the arguments passed to get rank , size and call my_server()*/
int MPI_Init(int *argc, char ***argv){
    struct sockaddr_in s_ad; /* struct to hold server address */
    char pch[500];
    char *token;
    struct hostent *he;
    struct in_addr **addr_list;
    int i, j=0;
    if (*argc < 2) {
       // fprintf(stderr,"LIB usage: ./p1 size rank\n");
        return 1;
    }
    size =atoi(((*argv)[1]));
    rank =atoi(((*argv)[2]));
    strcpy(pch,(*argv)[3]);
  //  fprintf(stderr,"\n rank = %d , size = %d , compute string =%s", rank, size, pch);
        token = strtok(pch, ",");
        while(token!=NULL){
        strcpy(addr_map[j],token);
        j++;
        token = strtok(NULL,",");
    }
    //for(i =0 ; i < size ;i++)
       // fprintf(stderr,"%s\n",addr_map[i]);
    if(my_server()==0)
        fprintf(stderr,"server up and running for rank %d ", rank);
    return 0;
}
/*creates active listening  server sockets at each task*/
int my_server(){
    struct   hostent   *ptrh;     /* pointer to a host table entry */
    struct   protoent  *ptrp;     /* pointer to a protocol table entry */
    struct   sockaddr_in sad;     /* structure to hold server's address */
    int      port;                /* protocol port number */

    memset((char  *)&sad,0,sizeof(sad)); /* clear sockaddr structure   */
    sad.sin_family = AF_INET;            /* set family to Internet     */
    sad.sin_addr.s_addr = INADDR_ANY;    /* set the local IP address */
    sad.sin_port = htons(35657+rank);

    if ( ((intptr_t)(ptrp = getprotobyname("tcp"))) == 0)  {
              //  fprintf(stderr, "cannot map \"tcp\" to protocol number");
     }

     /* Create a socket */
     sd = socket (PF_INET, SOCK_STREAM, ptrp->p_proto);
     while (sd < 0) {
                sd = socket (PF_INET, SOCK_STREAM, ptrp->p_proto);
     }
     //fprintf(stderr,"LIB: socket creation successful %d\n",rank);
     /* Bind a local address to the socket */
     while(1){
                if (bind(sd, (struct sockaddr *)&sad, sizeof (sad)) >=0) {
            break;
                }
     }
     fprintf(stderr,"\n TEST:Server Bind success %d for rank %d",ntohs(sad.sin_port),rank);
     /* Specify a size of request queue */
        while(1){
                 if (listen(sd, QLEN) >=0) {
                         break;
                 }
        return 0;
}

}
