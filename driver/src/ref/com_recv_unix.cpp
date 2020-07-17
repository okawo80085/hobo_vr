#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <string>
// #include <sys/types.h>
#include <sys/socket.h>
// #include <netinet/in.h>
#include <netdb.h> 

void error(const char *msg)
{
    perror(msg);
    exit(0);
}

int main(int argc, char *argv[])
{

    char buffer[256];
    if (argc < 3) {
       fprintf(stderr,"usage %s hostname port\n", argv[0]);
       exit(0);
    }

    // ###########socket init############
    char* my_addr = "127.0.0.1"; // just a simple char array, why does this have to be some incompatible with everything type in winsock?
    char* my_port = "6969"; // same as my_addr


    // make port and addr
    int sockfd; // the socket obj is just an int
    int portno, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;

    server = gethostbyname(my_addr); // oh and did i mention that this is also convoluted as shit in winsock?
    portno = atoi(my_port);


    sockfd = socket(AF_INET, SOCK_STREAM, 0); // create socket, surprisingly winsock didn't fuck this up
    if (sockfd < 0) 
        error("ERROR opening socket");

    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }

    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;

    // a long ass function call just to copy the addr into a socket addr struct, still better then winsock
    bcopy((char *)server->h_addr, 
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);
    serv_addr.sin_port = htons(portno); // copy port to the same struct
    if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) // connect and check if successful
        error("ERROR connecting");
    // ##################################



    // ##########msg send################
    printf("Please enter the message: ");
    bzero(buffer,256);
    fgets(buffer,255,stdin);// fill buffer with input from console

    n = write(sockfd,buffer,strlen(buffer)); // send buffer
    if (n < 0)
         error("ERROR writing to socket");
    // ##################################

    // ##########msg read################
    bzero(buffer,256);
    n = read(sockfd,buffer,255); // read 255 into buffer
    if (n < 0) 
         error("ERROR reading from socket");
    printf("%s\n",buffer);
    // ##################################


    // ##########close connection########
    n = write(sockfd,"CLOSE\n",strlen("CLOSE\n")); // hobo_vr server close message
    close(sockfd); // close socket, why does this have to take like a million lines with winsock?
    // ##################################
    return 0;
}

// overall way fucking simpler then the winsock
// winsock is a crime to humanity, avoid it at all costs
// i wouldn't use it too, but this shit has to work on windows
// and of course microsoft had to make winsock the way they did

// did i mention that unix sockets have built in buffer type conversion read?
// you can no joke pass a float buffer 
// and it will fill it floats interpreted from the message!

// but oh no microsoft had to be not like everyone else
// they had had to make it harder for everyone
// and lock it to char only buffers for winsock



// fuck winsock