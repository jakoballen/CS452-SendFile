//
//  main.cpp
//  sendfile_combined
//
//  Created by Jakob Allen on 11/18/20.
//

#include <iostream>
#include <unistd.h>
#include <cstdio>
#include <sys/socket.h>
#include <cstdlib>
#include <netinet/in.h>
#include <cstring>
#include <netdb.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <fstream>

#define SA struct sockaddr

//method headers
int setupServerSocket(int portno);
int serverSocketAccept(int serverSocket);
void printMD5(char* fileName);
void substring(char* s, char* substring, int p, int l);
void print_hex(const char *string);
int getPort();
char* getIP();
char* getFileName();
int getPacketSize();
FILE* openFile(char* fileName);
int callServer(char *host, int portno); // connect to a server
unsigned long getFileSize(int fd);
char* getKey();
void runEncryptionKey(char* content, char* key, unsigned long len);
void sendFile(int sockfd, char* buf, int packetSize, unsigned long fileSize);
void recvFile(int connfd, char* buf, int packetSize, unsigned long fileSize);

int main(int argc, const char * argv[]) {
    char* ip = getIP();
    int port = getPort();
    unsigned long size = 0;
    int pktSize = 0;
    char* buf;
    char* fileName;
    char* key;
    
    if(argc > 1 && strcmp(argv[1], "client")==0){
        std::cout << "File to be sent: ";
        fileName = getFileName();
        pktSize = getPacketSize();
        key = getKey();
        FILE* file = openFile(fileName);
        size = getFileSize(fileno(file));
        int sockfd = callServer(ip, port);
        write(sockfd, &pktSize, sizeof(int));
        write(sockfd, &size, sizeof(unsigned long));
        buf = (char*) malloc(sizeof(char)*size);
        fread(buf, 1, size, file);
        runEncryptionKey(buf, key, size);
        sendFile(sockfd, buf, pktSize, size);
        fclose(file);
    }else if(argc > 1 && strcmp(argv[1], "server")==0){
        std::cout << "Save file to: ";
        fileName = getFileName();
        std::cout<<"\n";
        key = getKey();
        int sockfd = setupServerSocket(port);
        int connfd = serverSocketAccept(sockfd);
        read(connfd, &pktSize, sizeof(int));
        read(connfd, &size, sizeof(unsigned long));
        buf = (char*) malloc(sizeof(char)*size);
        recvFile(connfd, buf, pktSize, size);
        runEncryptionKey(buf, key, size);
        FILE *newFile = fopen(fileName, "wb");
        fwrite(buf, sizeof(char), size, newFile);
        fclose(newFile);
    }
    
    if(argc>1){
        printMD5(fileName);
        free(buf);
        free(fileName);
        free(key);
        free(ip);
    }
    return 0;
}

/**
 * Receive the file from the client
 * @param connfd socket file descriptor
 * @param buf buffer for input
 * @param packetSize size of packets in KB
 * @param fileSize size of file in bytes
 */
void recvFile(int connfd, char* buf, int packetSize, unsigned long fileSize){
    int totalRecvd = 0;
    char* packet = (char*)malloc(sizeof(char)*(packetSize+1));
    int recvd = 0;
    int pktNum = 0;
    while(totalRecvd < fileSize){
        recvd = recv(connfd, buf+totalRecvd, packetSize, 0);
        
        if(pktNum<2 || pktNum > (fileSize/packetSize)-2){
            substring(buf, packet, totalRecvd, packetSize);
            printf("Rec packet #%d - encrypted as ", pktNum);
            print_hex(packet);
            
        }
        if(pktNum == 2){
            printf("\t\t\t.\n\t\t\t.\n\t\t\t.\n\t\t\t.\n");
        }
        pktNum++;
        
        totalRecvd += recvd;
    }
    printf("Receive success!\n");
}


/**
 * Send the file to the server
 * @param sockfd socket file descriptor
 * @param buf buffer for output
 * @param packetSize size of packets in KB
 * @param fileSize size of file in bytes
 */
void sendFile(int sockfd, char* buf, int packetSize, unsigned long fileSize){
    int totalSent = 0;
    int pktNum = 0;
    char* packet = (char*)malloc(sizeof(char)*(packetSize+1));
    while(totalSent < fileSize){
        int sent =write(sockfd,buf+totalSent, packetSize);
        if(pktNum<2 || pktNum > (fileSize/packetSize)-2){
            substring(buf, packet, totalSent, packetSize);
            printf("Sent packet #%d - encrypted as ", pktNum);
            print_hex(packet);
        }
        if(pktNum == 2){
            printf("\t\t\t.\n\t\t\t.\n\t\t\t.\n\t\t\t.\n");
        }
        totalSent += sent;
        pktNum++;
    }
    std::cout << "Send success!\n";
}

/**
 * Encrypt/decrypt the buffer
 * @param content content to be encrypted/decrypted
 * @param len length of the content
 */
void runEncryptionKey(char* content, char* key, unsigned long len){
    for(int i = 0; i<len; i++){
        content[i] = content[i]^key[i%strlen(key)];
    }
}

/**
 * Open the file and return a file pointer
 * @param fileName name of the file
 */
FILE* openFile(char* fileName){
    FILE* file = fopen(fileName, "rb");
    if(!file){
        std::cout << "Error while reading the file.\n";
    }
    return file;
}

/**
 * Prompt the user for the key
 * @return pointer to key
 */
char* getKey(){
    char* key = (char*)calloc(50, sizeof(char));
    std::cout<<"Enter encryption key: ";
    scanf("%s", key);
    std::cout<<"\n";
    return key;
}

/**
 * Prompt the user for their desired packet size
 * @return int value of their input
 */
int getPacketSize(){
    int pktSize = 0;
    std::cout<<"Pkt size: ";
    scanf("%d", &pktSize);
    return pktSize;
}


/**
 * Prompt the user for the file name
 * @return pointer to file name
 */
char* getFileName(){
    char* fileName = (char*)calloc(100, sizeof(char));
    scanf("%s", fileName);
    return fileName;
}

/**
 * Prompt the user for the IP address
 * @return pointer to ip
 */
char* getIP(){
    std::cout << "Connect to IP address: ";
    char* ip = (char*)calloc(16, sizeof(char));
    scanf("%s", ip);
    return ip;
}

/**
 * Prompt the user for the port
 * @return int value of the port
 */
int getPort(){
    int port = 0;
    std::cout<<"Port#: ";
    scanf("%d", &port);
    return port;
}


/**
 * Print the hex value of the given string
 */
void print_hex(const char *string){
    unsigned char *p = (unsigned char *) string;
    printf("%02x%02x...%02x%02x\n", p[0], p[1], p[strlen(string)-2], p[strlen(string)-1]);
}

/**
 * Copy a substring to the passed buffer
 * @param s original string
 * @param substring resulting substring
 * @param p starting index
 * @param l length of substring
 */
void substring(char* s, char* substring, int p, int l) {
   int c = 0;
   while (c < l) {
      substring[c] = s[p+c-1];
      c++;
   }
   substring[c] = '\0';
}

/**
 * Print the md5 sum of the passed file name
 */
void printMD5(char* fileName){
    char* cmd = (char*) calloc(strlen(fileName)+7+1, sizeof(char));

    strcpy(cmd, "md5sum ");
    strcat(cmd, fileName);
    system(cmd);
    free(cmd);
}

//set up the socket to listen on
int setupServerSocket(int portno){
    struct sockaddr_in servaddr;
    // socket create and verification
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        printf("Socket creation failed...\n");
        exit(0);
    }
    bzero(&servaddr, sizeof(servaddr));

    // assign IP, PORT
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(portno);

    // Binding newly created socket to given IP and verification
    if ((bind(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr))) != 0) {
        printf("Socket bind failed...\n");
        exit(0);
    }

    // Now server is ready to listen and verification
    if ((listen(sockfd, 5)) != 0) {
        printf("Listen failed...\n");
        exit(0);
    }
    return sockfd;
}

int serverSocketAccept(int serverSocket){
    struct sockaddr_in cli;
    int len = sizeof(cli);
    // Accept the data packet from client and verification
    int connfd = accept(serverSocket, (struct sockaddr *)(&cli), (socklen_t*)(&len));
    if (connfd < 0) {
        printf("Server acccept failed...\n");
        exit(0);
    }
        return connfd;
}

//connect to the server
int callServer(char* host, int portno){
    int sockfd;
    struct sockaddr_in servaddr;
    // socket create and verification
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        std::cout << "socket creation failed...\n";
        exit(0);
    }
    bzero(&servaddr, sizeof(servaddr));

    // assign IP, PORT
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr(host);
    servaddr.sin_port = htons(portno);

    // connect the client socket to server socket
    if (connect(sockfd, (SA*)&servaddr, sizeof(servaddr)) != 0) {
        printf("connection with the server failed...\n");
        exit(0);
    }
    return sockfd;
}

unsigned long getFileSize(int fd){
    struct stat stat_buf;
    int rc = fstat(fd, &stat_buf);
    return rc == 0 ? stat_buf.st_size : -1;
}
