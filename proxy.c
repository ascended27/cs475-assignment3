#include <stdio.h>
#include <string.h>
#include "csapp.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

typedef struct _request{
    char request[MAXBUF];
    char method[16];
    char path[MAXLINE];
    char version[16];
    char contentType[128];
    char host[MAXLINE];
    char page[MAXLINE];
    char conn[6];
    char proxyConn[6];
} request;

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";

int main(int argc, char** argv)
{

    int listenfd,connfd,n;
    rio_t rio;
    socklen_t clientlen;
    struct sockaddr_storage clientAddr;
    char buf[MAXLINE];

    // If we have a port passed in continue
    if(argc == 2){
        // Open socket to listen on
        listenfd = Open_listenfd(argv[1]);

        // Notify user that proxy is running
        printf("Proxy running on port: %s\n", argv[1]);

        // Listen loop
        while(1){
            // Create a socket to read from
            connfd = sizeof(struct sockaddr_storage);
            // Block until we get a connection
            connfd = Accept(listenfd, (SA *)&clientAddr, &clientlen);

            // Array of extra headers that we need to add
            char extras[20][MAXLINE];
            // Count of extras in the array
            int extraCount=0;

            // Request struct to store all the request info
            request* req = malloc(sizeof(request));

            // Connection & Proxy-Connection always need to be close
            strcpy(req->conn,"close");
            strcpy(req->proxyConn,"close");

            // Init rio
            Rio_readinitb(&rio, connfd);

            // Read lines until there are no more
            while((n=Rio_readlineb(&rio,buf,MAXLINE)) != 0){
                // Copy our buffer so we have a buffer to tokenize
                char bufCpy[MAXBUF];
                strcpy(bufCpy,buf);

                // Pointers for string manipulation
                char* bufPtr;
                char* tok;

                // Get the first token
                tok = strtok_r(bufCpy," ",&bufPtr);

                // If it is post or get the we have the first line
                if(strcmp(tok,"GET")==0 || strcmp(tok,"POST")==0){
                    // Copy Method from the buffer
                    strcpy(req->method,tok);

                    // Get the next token and copy it to the path
                    tok = strtok_r(bufPtr," ",&bufPtr);
                    strcpy(req->path,tok);

                    //TODO: This probably needs to change since we only ever send with HTTP/1.0
                    // Get the next token and copy it to the version.
                    tok = strtok_r(bufPtr," ",&bufPtr);
                    strcpy(req->version,tok);
                } else if(strcmp(tok,"Host:")==0){ // If our token is Host
                    // Get the value of Host and store it in the request host
                    tok = strtok_r(bufPtr," ",&bufPtr);
                    strcpy(req->host,tok);
                } else if(strcmp(tok,"Proxy-Connection:")==0){ // If our token is Proxy-Conn
                    // We don't need to do anything we are always sending close for this
                } else if(strcmp(tok,"Connection:")==0){ // If our token is Connection
                    // We don't need to do anything we are always sending close for this
                }else if(strcmp(tok,"User-Agent:")==0){ // If our token is User-Agent
                    // We don't need to do anything we are always sending user_agent_hdr for this
                } else{ // Otherwise we have an extra
                    // Add the extra to the extras array
                    strcpy(extras[extraCount++],buf);
                }
                // Clear our buffer out
                memset(buf,0,sizeof(buf));
            }

            // TODO: REMOVE THIS
            // This is just debug prints
            printf("%s ",req->method);
            printf("%s ",req->path);
            printf("%s",req->version);
            printf("Host: %s",req->host);
            printf("User-Agent: %s",user_agent_hdr);
            printf("Connection: %s\n", req->conn);
            printf("Proxy-Connection: %s\n", req->proxyConn);

            int i;
            for(i = 0; i < extraCount; i++)
                printf("%s",extras[i]);

            // TODO: Forward this to the specified host
            // Connection is done for now so just close it.
            Close(connfd);
        }

        exit(0);
    }

    return 0;
}
