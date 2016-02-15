#include "myrtt.h"
#include<stdio.h>
#include<stdlib.h>
#include<time.h>
#include<math.h>
#include<string.h>
/* function declarations */
char* get_message(int);
double standard_deviation(double size_rtt,double rtt[]);
int         main(int, char**);


int main (int argc, char *argv[])
{
  //printf("Running code\n");
  /* process information */
  int   numproc, rank, len;
  MPI_Status status;
  /* record times to calculate RTT */
  double send_time,recv_time;
  /* Loop iterators */
  int i,j,k,l;
  /* Current message size and the node pair*/
  int rank_pair,curr_size;
  /* Message */
  char* message;
  /* ACK to ensure the next pair of nodes start communication */
  char ack='a';
  /* Sum to calculate average across 10 messages of the same size */
  double sum = 0;
  /* RTT for each of the 10 messages sent */
  double rtt[9];
  /* Average RTT and Standard Error for each message size */
  double average_rtt[16],std_dev_rtt[16];
  //printf("Before Init\n");
  /* initialize MPI */
  MPI_Init(&argc, &argv);
  /* get the number of procs in the comm */
  MPI_Comm_size(MPI_COMM_WORLD, &numproc);
  /* get my rank in the comm */
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  //printf("rank : %d np :  %d\n",rank,numproc);
  double pair_message_size[numproc/2],pair_average_rtt[numproc/2][17],pair_std_dev[numproc/2][17];
  /* All nodes with even rank can start only after the previous pair finishes communication
     MPI_Recv is a blocking call and hence waits until it receives the ACK to start
  */
  if(rank != 0 && rank%2 == 0)
	MPI_Recv(&ack,1,MPI_CHAR,rank-1,20,MPI_COMM_WORLD,&status);
  //printf("TEST :Starting for rank : %d\n",rank);	
  /* Run for each message size starting from 32 bytes to 2MB */
  for(i = 0 ; i <= 16 ; i++)
	{
		//printf("Running in %d for another size\n",rank);
		/* Get message with current size */
		//printf("Prcessing in process with rank %d\n",rank);
		message = get_message(i);
		/* Send and Receive same message 10 times to calculate RTT */
		for(j = 0 ; j < 10 ; j++) 
		{
			//printf("Message contents : %c \n",message[0]);
			/* Current message size */
			curr_size = pow(2,i+5);
			//printf("Running in %d for size %lf\n",rank,curr_size);
			/*
			  For all odd ranks, Receive message from its previous node and send back
		          and vice versa for even nodes to ensure synchronization
			*/
			if(rank % 2 == 1){
				rank_pair=rank-1;
				//printf("Process %d receiving from process %d\n",rank,rank-1);
				//gettimeofday(&send_time, NULL);
				/* Calculate Start time for sending message*/
				send_time = MPI_Wtime();
				MPI_Recv(message,curr_size,MPI_CHAR,rank-1,10,MPI_COMM_WORLD,&status);
  				//printf("TEST :Receivedd from rank : %d\n",rank-1);	
				//printf("Process %d sending to process %d",rank,rank+1);
				MPI_Send(message,curr_size,MPI_CHAR,rank-1,10,MPI_COMM_WORLD);
  				//printf("TEST :Sent to rank : %d\n",rank-1);	
				//gettimeofday(&recv_time, NULL);
				/* Calculate Received time for receiving message*/
				recv_time = MPI_Wtime();
				//if(rank != numproc - 1)
				//MPI_Send(&ack,1,MPI_CHAR,rank+1,20,MPI_COMM_WORLD);
				//printf("Process %d sent pipe message to process %d\n",rank,rank+1);
			}
			else if(rank % 2 == 0){
				rank_pair=rank+1;
				//printf("Process %d receiving pipe message from process %d\n",rank,rank-1);
				//if(rank != 0)
				//	MPI_Recv(&ack,1,MPI_CHAR,rank-1,20,MPI_COMM_WORLD,&status);
				//gettimeofday(&send_time, NULL);
				/* Calculate Start time for sending message*/
				send_time = MPI_Wtime();
				//printf("Process %d sending to process %d\n",rank,rank+1);
				MPI_Send(message,curr_size,MPI_CHAR,rank+1,10,MPI_COMM_WORLD);
  				//printf("TEST :Sent to rank : %d\n",rank+1);	
				//printf("Process %d receiving from process %d\n",rank,rank-1);
				MPI_Recv(message,curr_size,MPI_CHAR,rank+1,10,MPI_COMM_WORLD,&status);
  				//printf("TEST :Received from  rank : %d\n",rank+1);	
				/* Calculate Received time for receiving message*/
				recv_time = MPI_Wtime();
				//gettimeofday(&recv_time, NULL);
			}
			//printf("CALCULATING AVERAGES\n");			
 			/* Skip recording first message for RTT 
			   as it takes more time than the rest to
			   initiate the communication
			*/
			if(j != 0 && rank%2 == 1)
			{
				rtt[j-1] = recv_time - send_time;
				//printf("Time taken for RTT from rank %d is %e with recvt time %e and send time %e\n",rank,rtt[j-1],recv_time,send_time);
				sum+=rtt[j-1];
			}	
		}
		/* For each pair of nodes only the odd nodes calculate the RTT */
		if(rank%2 == 1) {
			//printf("SUM : %e\n",sum);
			average_rtt[i]=sum/9;
			//printf("TEST : AVERAGE RTT AT RANK %d is %e\n",rank,average_rtt[i]);
			std_dev_rtt[i] = standard_deviation(average_rtt[i],rtt);
			//printf("%lf\t",pow(2,i+5));
			curr_size = pow(2,i+5);
			//printf("%lf\t",pow(2,i+5));
			//if(rank_pair > rank)
			//printf("(%d,%d)\t%lf\t",rank,rank_pair,average_rtt[i]);
			//else
			//	printf("(%d,%d)\t%lf\t",rank_pair,rank,average_rtt[i]);
				
			//printf("%lf\n",std_dev_rtt[i]);
			sum = 0;
		}
	}
 //printf("HECK, AM DONE BABY\n");
  if(rank%2 == 1){
	//printf("RANK %d sending final stats to rank 0\n",rank);
  	MPI_Send(&curr_size,1,MPI_DOUBLE,0,20,MPI_COMM_WORLD);
  	MPI_Send(&average_rtt,17,MPI_DOUBLE,0,30,MPI_COMM_WORLD);
  	MPI_Send(&std_dev_rtt,17,MPI_DOUBLE,0,40,MPI_COMM_WORLD);
	//printf("RANK %d finished sending final stats to rank 0\n",rank);
  }
  if(rank == 0){
	//printf("Rank 0 receiving from other nodes\n");
	for(k = 0 ; k < numproc/2;k++){
		//printf("Rank 0 receiving from %d\n",2*k+1);
		MPI_Recv(&pair_message_size[k],1,MPI_DOUBLE,(2*k+1),20,MPI_COMM_WORLD,&status);
		MPI_Recv(&pair_average_rtt[k],17,MPI_DOUBLE,(2*k+1),30,MPI_COMM_WORLD,&status);
		MPI_Recv(&pair_std_dev[k],17,MPI_DOUBLE,(2*k+1),40,MPI_COMM_WORLD,&status);
		//printf("Rank 0 finished receiving from %d\n",2*k+1);
		//printf("Received from process %d\t%lf\t%lf\t%lf\n",(2*k)+1,pair_message_size[k][i],pair_average_rtt[k][i],pair_std_dev[k][i]);
	}
  }
  /* All odd nodes need to ACK to the next nodes for the next pair to communicate*/
  if(rank != numproc - 1 && rank%2 == 1)
	MPI_Send(&ack,1,MPI_CHAR,rank+1,20,MPI_COMM_WORLD);
  if(rank == 0){
	//printf("Rank 0 trying to print out final values\n");
	for(l=0;l<=16;l++){
		printf("%lf\t",pow(2,l+5));
		for(k = 0;k < numproc/2;k++){
			printf("%e\t%e\t",pair_average_rtt[k][l],pair_std_dev[k][l]);
		}
		printf("\n");
	}
  }
  /* MPI Cleanup*/
  MPI_Finalize();
}

/* Generate Message for current iteration*/
char* get_message(int i){
	int j = i+5;
	double sizes =pow(2,j);
	char *msg;
	msg = (char*) malloc(sizes*sizeof(char));
	memset(msg, 'a', sizes * sizeof(char));
	return msg;
}
/* Calculate standard deviation for the set of data*/
double standard_deviation(double size_rtt,double rtt[]){
	double sum = 0;
	int i = 0;
	double avg,std_dev;
	for(i = 0 ; i < 9 ; i++){
		sum += pow((rtt[i] - size_rtt),2);
	}
	avg = sum/9;
	std_dev = sqrt(avg);
	return std_dev;
}
