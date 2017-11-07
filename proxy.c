#include <stdio.h>
#include <string.h>
#include "csapp.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

typedef struct _request{
    char method[16];
    char path[MAXLINE];
    char version[16];
    char contentType[128];
    char host[MAXLINE];
    char port[MAXLINE];
    char conn[6];
    char proxyConn[6];
    char extras[20][MAXLINE];
    int  extraCount;
} request;

typedef struct _threadArgs{
    int fd;
    struct sockaddr_in sock;
    socklen_t clientlen;
} threadArgs;

void parse(rio_t*,int, request*);
void forward(rio_t*, request*);
void* thread(void* argv);

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";

int main(int argc, char** argv)
{
    int listenfd,connfd;
    socklen_t clientlen;
    struct sockaddr_in clientAddr;
    pthread_t tid;

    // If we have a port passed in continue
    if(argc == 2){
        // Open socket to listen on
        listenfd = Open_listenfd(argv[1]);

        // Notify user that proxy is running
        printf("Proxy running on port: %s\n", argv[1]);

        // Listen loop
        while(1){
            // Create a socket to read from
            clientlen = sizeof(clientAddr);
            threadArgs* args = (threadArgs*) malloc(sizeof(threadArgs));
            // Block until we get a connection
            connfd = Accept(listenfd, (SA *)(&(args->sock)), &clientlen);
            printf("New conn\n");
            args->fd = connfd;
            Pthread_create(&tid,NULL,thread,(void *) args);
//            Close(connfd);
        }

        exit(0);
    }

    return 0;
}

void* thread(void* argv){
        rio_t rio;


        Pthread_detach(pthread_self());

        threadArgs* args = (threadArgs*) argv;
        int fd = args->fd;
        struct sockaddr_in sock;
        memcpy(&sock,&(args->sock),sizeof(struct sockaddr_in));
        free(args);

        // Init rio
        Rio_readinitb(&rio, fd);

        // Request struct to store all the request info
        request* req = malloc(sizeof(request));

        // Connection & Proxy-Connection always need to be close
        strcpy(req->conn,"close");
        strcpy(req->proxyConn,"close");
        req->extraCount=0;

        parse(&rio,fd,req);

        // TODO: REMOVE THIS
        // This is just debug prints
        printf("%s ",req->method);
        printf("%s ",req->path);
        printf("%s\n",req->version);
        printf("Host: %s",req->host);
        printf("User-Agent: %s",user_agent_hdr);
        printf("Connection: %s\n", req->conn);
        printf("Proxy-Connection: %s\n", req->proxyConn);

        int i;
        for(i = 0; i < req->extraCount; i++)
            printf("%s",req->extras[i]);

        forward(&rio,req);

        // TODO: Forward this to the specified host
        // Connection is done for now so just close it.
        Close(fd);
        return NULL;
}

void parse(rio_t* rio, int connfd, request* req){
    int n;
    char* bufPtr;
    char* tok;


    // Request buffer
    char buf[MAXLINE];

    char method[MAXLINE],uri[MAXLINE],version[MAXLINE],tmp[MAXLINE];

    // Get the method, uri, and version
    if((n=Rio_readlineb(rio,buf,MAXLINE)) != 0){
            sscanf(buf,"%s %s %s",method,uri,version);
            if(strcasecmp(method,"GET")){
                // TODO: Make this respond to user with error 501
                printf("Error only support method: GET\n");
            } else{
                strcpy(req->method,method);
                strcpy(req->path,uri);
                strcpy(req->version,"HTTP/1.0");
            }
    }

    // Read lines until there are no more
    while((n=Rio_readlineb(rio,buf,MAXLINE)) != 0){

        // Copy our buffer so we have a buffer to tokenize
        char bufCpy[MAXBUF];
        strcpy(bufCpy,buf);

        // Pointers for string manipulation

        // Get the first token
        tok = strtok_r(bufCpy," ",&bufPtr);

        if(strcmp(tok,"Host:")==0){ // If our token is Host
            // Get the value of Host and store it in the request host
            tok = strtok_r(bufPtr," ",&bufPtr);
            tok[strcspn(tok,"\r\n")]=0;
            strcpy(req->host,tok);
            strcpy(tmp,req->host);
            tok = strtok_r(tmp,":",&bufPtr);
            strcpy(req->host,tok);
            strcpy(req->port,bufPtr);
        } else if(strcmp(tok,"Proxy-Connection:")==0){ // If our token is Proxy-Conn
            // We don't need to do anything we are always sending close for this
        } else if(strcmp(tok,"Connection:")==0){ // If our token is Connection
            // We don't need to do anything we are always sending close for this
        }else if(strcmp(tok,"User-Agent:")==0){ // If our token is User-Agent
            // We don't need to do anything we are always sending user_agent_hdr for this
        } else{ // Otherwise we have an extra
            // Add the extra to the extras array
            strcpy(req->extras[req->extraCount++],buf);
        }
        // Clear our buffer out
        memset(buf,0,sizeof(buf));
    }
}

void forward(rio_t* rio, request* req){

    // TODO: Forward the request to the target server

}