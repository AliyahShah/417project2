#include <stdio.h>
#include <string.h>	/* for using strtok, strcmp, etc */
#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "common.h"
#define RCVBUFSIZE 32 /* Size of receive buffer */
int servSock; /* Socket descriptor for server */
    int clntSock; /* Socket descriptor for client */
    struct sockaddr_in echoServAddr; /* Local address */
    struct sockaddr_in echoClntAddr; /* Client address */
    unsigned short echoServPort; /* Server port */
    unsigned int clntLen; /* Length of client address data structure */ 
    char echoBuffer[RCVBUFSIZE];
    int recvMsgSize;
    char *id;
    char *name;
    int cookie;
    
int main (int argc, char **argv){
    if (argc != 2) /* Test for correct number of arguments */ {
        fprintf(stderr, "Wrong number of args %d \n", argc);
        fflush(stderr);
		exit(1);
    } 

    fflush(stdout);
    fflush(stderr);
    echoServPort = atoi(argv[1]); /* First arg: local port */ 
    
    if ((servSock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        fprintf(stderr, "error making socket\n");
        fflush(stderr);
    }
        
    /* Construct local address structure */
    memset(&echoServAddr, 0, sizeof(echoServAddr)); /* Zero out structure */
    echoServAddr.sin_family = AF_INET; /* Internet address family */
    echoServAddr.sin_addr.s_addr = htonl(INADDR_ANY); /* Any incoming interface */
    echoServAddr.sin_port = htons(echoServPort); /* Local port */ 
    
    /* Bind to the local address */
    if (bind(servSock, (struct sockaddr *)&echoServAddr, sizeof(echoServAddr)) < 0){ 
        fprintf(stderr, "error binding socket\n");
        close(servSock);
        fflush(stderr); 
        fflush(stdout);
        close(clntSock);
        exit(1);
    }
    
    /* Mark the socket so it will listen for incoming connections */
    if (listen(servSock, MAXPENDING) < 0)
        fprintf(stderr, "error listening?\n");
    
    for (;;) /* Run forever */ {
        /* Set the size of the in-out parameter */
        clntLen = sizeof(echoClntAddr);
        
        /* Wait for a client to connect */
        if ((clntSock = accept(servSock, (struct sockaddr *) &echoClntAddr, &clntLen)) < 0){
            fprintf(stderr, "**Error** from %s:%d\n", inet_ntoa(echoClntAddr.sin_addr),echoServPort);
            fflush(stdout);   
        }
            
        /* Receive HELLO message from client */
        if ((recvMsgSize = recv(clntSock, echoBuffer, RCVBUFSIZE, 0)) < 0){
            fprintf(stderr, "**Error** from %s:%d\n", inet_ntoa(echoClntAddr.sin_addr),echoServPort);
            fflush(stdout);
        }
        
        /* Get HELLO message from client*/
        if (recvMsgSize > 0) /* zero indicates end of transmission */{
            /*get hello msg. return -1 if error*/
            if(rcvHello() == 0){
                /* send STATUS message back to client */
                char sendBack[80] = "";
                strcat(sendBack, MAGIC_STRING);
                strcat(sendBack, " STATUS ");
                
                cookie = makeCookie(inet_ntoa(echoClntAddr.sin_addr));
                sprintf(sendBack,"%s %d %s:%d",sendBack,cookie,inet_ntoa(echoClntAddr.sin_addr),echoServPort );
                //printf("sendback: %s\n",sendBack);
                
                //print cookie id name from ip:port
                fprintf(stdout, "%d %s %s from %s:%d\n", cookie, id, name, inet_ntoa(echoClntAddr.sin_addr),echoServPort);
                fflush(stdout);
               
                	    
                if (send(clntSock, sendBack, recvMsgSize, 0) != recvMsgSize){/*send first status message back*/
                    fprintf(stderr, "**Error** from %s:%d\n", inet_ntoa(echoClntAddr.sin_addr),echoServPort);
                    fflush(stderr);   
                    fflush(stdout);
                }
            } 
        }
        
        
        /*Get bye message*/
        if ((recvMsgSize = recv(clntSock, echoBuffer, RCVBUFSIZE, 0)) < 0){
            //client socket has been closed
            //fprintf(stderr, "recieve failed\n");
        } else { /*continue to get bye and send bye messages*/
            if (recvMsgSize > 0) {
                if(rcvBye() == 0){/*if recieved correct message, send bye */
                    /* send SERVER_BYE message back to client */
                    char sendBack[50] = "";
                    strcat(sendBack, MAGIC_STRING);
                    strcat(sendBack, " SERVER_BYE");
                    if (send(clntSock, sendBack, recvMsgSize, 0) != recvMsgSize){
                        fprintf(stderr, "**Error** from %s:%d\n", inet_ntoa(echoClntAddr.sin_addr),echoServPort);
                        fflush(stderr);  
                        fflush(stdout);
                    }
                }
            }
        }
        close(clntSock); /* Close client socket */ 
    } 
    
    close(clntSock);
    close(servSock);
    return 0;
}

/*taken from a stack overflow post*/
int is_empty(const char *s) {
  while (*s != '\0') {
    if (!isspace((unsigned char)*s))
      return 0;
    s++;
  }
  return 1;
}

int makeCookie(int ip){
   unsigned char bytes[4];
    bytes[0] = ip & 0xFF;
    bytes[1] = (ip >> 8) & 0xFF;
    bytes[2] = (ip >> 16) & 0xFF;
    bytes[3] = (ip >> 24) & 0xFF;   
    //printf("%d.%d.%d.%d\n", bytes[3], bytes[2], bytes[1], bytes[0]);
    return (((bytes[3] + bytes[2] + bytes[1] + bytes[0])*13)%1111);
}

int rcvHello(){
    //Validate message recieved by echoBuffer
    char * helloMsg = strtok (echoBuffer, " ");
    
    int count = 0;
    while(count <= 3){/*counting the number of arguments and parsing the incoming msg*/
    	if(count == 0 && strcmp(MAGIC_STRING,helloMsg) != 0){/*if incorrect magin string - error*/
    	   fprintf(stderr, "**Error** from %s:%d\n", inet_ntoa(echoClntAddr.sin_addr),echoServPort);
    	   fflush(stderr);  
    	    fflush(stdout);
    	   close(clntSock);
    	   return -1;
    	} else if(count == 1 && strcmp("HELLO",helloMsg) != 0){/*if not a hello msg = error*/
    	   fprintf(stderr, "**Error** from %s:%d\n", inet_ntoa(echoClntAddr.sin_addr),echoServPort);
    	   fflush(stderr);  
    	    fflush(stdout);
    	   close(clntSock);
    	   return -1;
        }else if(count == 2) {
    	    /*get the id and the name*/
    	    id = helloMsg;
    	} else if(count == 3){
    	    //get name last field
    	    name = helloMsg;
    	}
        
    	helloMsg = strtok(NULL, " ,.\r\r\n");
    	count++;
    }
    
    if(helloMsg != NULL && is_empty(helloMsg) != 0){ /*Wrong number of arguments sent in hellomsg*/
        fprintf(stderr, "**Error** from %s:%d\n", inet_ntoa(echoClntAddr.sin_addr),echoServPort);
        fflush(stderr);
         fflush(stdout);
        close(clntSock);
        return -1;
    } 
            
    return 0;
}

int rcvBye(){
    //Validate message recieved by echoBuffer
                char * helloMsg = strtok (echoBuffer, " ");
                
                int count = 0;
                while(count <= 3){/*counting the number of arguments and parsing the incoming msg*/
                	if(count == 0 && strcmp(MAGIC_STRING,helloMsg) != 0){/*if incorrect magic string - error*/
                	   fprintf(stderr, "**Error** from %s:%d\n", inet_ntoa(echoClntAddr.sin_addr),echoServPort);
                	   fflush(stderr);  
                	    fflush(stdout);
                	   close(clntSock);
                	   return -1;
                	} else if(count == 1 && strcmp("CLIENT_BYE",helloMsg) != 0){/*if not a client_bye msg = error*/
                	   fprintf(stderr, "**Error** from %s:%d\n", inet_ntoa(echoClntAddr.sin_addr),echoServPort);
                	   fflush(stderr);  
                	    fflush(stdout);
                	   close(clntSock);
                	   return -1;
                	} else if(count == 2 && cookie != atoi(helloMsg)) { /*if incorrect cookie*/
                	   fprintf(stderr, "**Error** from %s:%d\n", inet_ntoa(echoClntAddr.sin_addr),echoServPort);
                	   fflush(stderr); 
                	    fflush(stdout);
                	   close(clntSock);
                	   return -1;
                	}
                    
                	helloMsg = strtok(NULL, " ,.\r\r\n");
                	count++;
                }
                
                if(helloMsg != NULL && is_empty(helloMsg) != 0){ /*Wrong number of arguments*/
                    fprintf(stderr, "**Error** from %s:%d\n", inet_ntoa(echoClntAddr.sin_addr),echoServPort);
                    fflush(stderr);
                     fflush(stdout);
                    close(servSock);
                    close(clntSock);
                    return -1;
                }
                return 0;
}




