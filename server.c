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
    char echoBuffer2[RCVBUFSIZE];
    int recvMsgSize;
    char *id;
    char *name;
    int cookie;
    
int main (int argc, char **argv){
    if (argc == 2) /* Test for correct number of arguments */ {
         echoServPort = atoi(argv[1]); /* First arg: local port */ 
    } else {
        echoServPort = SERVER_PORT;
    }
    
    if ((servSock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        fprintf(stderr, "error making socket\n");
        fflush(stderr);
    }
        
    /* Construct local address structure */
    memset(&echoServAddr, 0, sizeof(echoServAddr)); /* Zero out structure */
    echoServAddr.sin_family = AF_INET; /* Internet address family */
    echoServAddr.sin_addr.s_addr = htonl(INADDR_ANY); /* Any incoming interface */
    echoServAddr.sin_port = htons(echoServPort); /* Local port */ 
    
    int reuse;
    if (setsockopt(servSock, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse, sizeof(int)) == -1){
     printf("Reuse port Error\n");
    }

    /* Bind to the local address */
    if (bind(servSock, (struct sockaddr *)&echoServAddr, sizeof(echoServAddr)) < 0){ 
        printf("error binding socket\n");
        fflush(stdout);
        close(servSock);
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
            printf( "**Error** from %s:%d", inet_ntoa(echoClntAddr.sin_addr),echoServPort);
            fflush(stdout);   
        }
            
        /* Receive HELLO message from client */
        if ((recvMsgSize = recv(clntSock, echoBuffer, RCVBUFSIZE, 0)) < 0){
            printf("**Error** from %s:%d", inet_ntoa(echoClntAddr.sin_addr),echoServPort);
            fflush(stdout);
        }
        
        /* Get HELLO message from client*/
        if (recvMsgSize > 0) /* zero indicates end of transmission */{
            /*get hello msg. return -1 if error*/
            if(rcvHello() == 0){
                /* send STATUS message back to client */
                char * sendBack = calloc(2000, 0);
                
                cookie = makeCookie(inet_ntoa(echoClntAddr.sin_addr));
                sprintf(sendBack,"%s STATUS %d %s:%d\n", MAGIC_STRING,cookie,inet_ntoa(echoClntAddr.sin_addr),echoServPort);
                
                if (send(clntSock, sendBack, strlen(sendBack), 0) != strlen(sendBack)){/*send first status message back*/
                    printf("**Error** from %s:%d", inet_ntoa(echoClntAddr.sin_addr),echoServPort);
                    fflush(stdout);
                }
            } 
        }
        
        
        /*Get bye message*/
        if ((recvMsgSize = recv(clntSock, echoBuffer2, RCVBUFSIZE, 0)) < 0){
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
                        printf("**Error** from %s:%d", inet_ntoa(echoClntAddr.sin_addr),echoServPort);
                        fflush(stdout);
                    }else{
                        //print cookie id name from ip:port
                        fflush(stdout);
                        printf("%d %s %s from %s:%d\n", cookie, id, name, inet_ntoa(echoClntAddr.sin_addr),echoServPort);
                        fflush(stdout);
                    }
                }
            }
        }
        close(clntSock); /* Close client socket */ 
    } 
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

int makeCookie(char * ip){/*TODO*/
    char * cookiestr = strtok(ip, " .,");
    int count = 0;
    int ip1 = 0;
    int ip2 = 0;
    int ip3 = 0;
    int ip4 = 0;
    while(count <= 3){
        if(count == 0){
            ip1 = atoi(cookiestr);
        }else if(count == 1){
            ip2 = atoi(cookiestr);
        }else if(count == 2){
            ip3 = atoi(cookiestr);
        }else{
            ip4 = atoi(cookiestr);
        }
        cookiestr = strtok(NULL, " ,.");
        count++;
    }
    return (((ip1 + ip2+ ip3 + ip4)*13)%1111);
    
}

int rcvHello(){
    //Validate message recieved by echoBuffer
    char * helloMsg = strtok (echoBuffer, " ");
    
    int count = 0;
    while(count <= 3){/*counting the number of arguments and parsing the incoming msg*/
    	if(count == 0 && strcmp(MAGIC_STRING,helloMsg) != 0){/*if incorrect magin string - error*/
    	   printf("**Error** from %s:%d", inet_ntoa(echoClntAddr.sin_addr),echoServPort);
    	   fflush(stdout);
    	   close(clntSock);
    	   return -1;
    	} else if(count == 1 && strcmp("HELLO",helloMsg) != 0){/*if not a hello msg = error*/
    	   printf("**Error** from %s:%d", inet_ntoa(echoClntAddr.sin_addr),echoServPort);
    	    fflush(stdout);
    	   close(clntSock);
    	   return -1;
        }else if(count == 2) {
    	    /*get the id and the name*/
    	   // printf("hello id: %s\n", helloMsg);
    	    id = helloMsg;
    	} else if(count == 3){
    	    //get name last field
    	    name = helloMsg;
    	}
        
    	helloMsg = strtok(NULL, " ,.\r\r\n");
    	count++;
    }
    
    if(helloMsg != NULL && is_empty(helloMsg) != 0){ /*Wrong number of arguments sent in hellomsg*/
        printf("**Error** from %s:%d", inet_ntoa(echoClntAddr.sin_addr),echoServPort);
         fflush(stdout);
        close(clntSock);
        return -1;
    } 
            
    return 0;
}

int rcvBye(){
    //Validate message recieved by echoBuffer
    char * byeMsg = strtok (echoBuffer2, " .,");
                
    int count = 0;
    while(count < 3){/*counting the number of arguments and parsing the incoming msg*/
    	if(count == 0 && strcmp(MAGIC_STRING,byeMsg) != 0){/*if incorrect magic string - error*/
    	   printf("**Error** from %s:%d", inet_ntoa(echoClntAddr.sin_addr),echoServPort);
    	   fflush(stdout);
    	   close(clntSock);
    	   return -1;
    	} else if(count == 1 && strcmp("CLIENT_BYE",byeMsg) != 0){/*if not a client_bye msg = error*/
    	   printf("**Error** from %s:%d", inet_ntoa(echoClntAddr.sin_addr),echoServPort);
    	    fflush(stdout);
    	   close(clntSock);
    	   return -1;
    	} else if(count == 2 && cookie != atoi(byeMsg)) { /*if incorrect cookie*/
    	   printf("**Error** from %s:%d", inet_ntoa(echoClntAddr.sin_addr),echoServPort);
    	    fflush(stdout);
    	   close(clntSock);
    	   return -1;
    	}
            
    	byeMsg = strtok(NULL, " ,.\r\r\n");
    	count++;
    }
                
    if(byeMsg != NULL && is_empty(byeMsg) != 0){ /*Wrong number of arguments*/
        printf("**Error** from %s:%d", inet_ntoa(echoClntAddr.sin_addr),echoServPort);
         fflush(stdout);
        close(clntSock);
        return -1;
    }
    return 0;
}



