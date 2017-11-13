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
void forward(request*,int);
void* thread(void* argv);
void errorMsg(char* error);

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";

void sigpipe_handler(int sig){
    printf("SIGPIPE received and ignored");
    return;
}

int main(int argc, char** argv)
{
    int listenfd,connfd;
    socklen_t clientlen;
    struct sockaddr_in clientAddr;
    pthread_t tid;

    // Install SIGPIPE handler
    Signal(SIGPIPE,sigpipe_handler);

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

            // Initialize thread args to pass to the new thread
            threadArgs* args = (threadArgs*) malloc(sizeof(threadArgs));

            // Block until we get a connection
            connfd = Accept(listenfd, (SA *)(&(args->sock)), &clientlen);

            // Add the new socket to the thread args and create a new thread
            // to handle the connection
            args->fd = connfd;
            Pthread_create(&tid,NULL,thread,(void *) args);
        }

        exit(0);
    }

    return 0;
}

void* thread(void* argv){
    rio_t rio;

    // Detach the thread from the main thread so the main
    // thread doesn't need to handle resources after this
    // thread exits
    Pthread_detach(pthread_self());

    // Install SIGPIPE handler
    Signal(SIGPIPE,sigpipe_handler);

    // Convert passed in arguments to a threadArgs structure
    // So we can extract the passed in thread arguments.
    threadArgs* args = (threadArgs*) argv;
    int fd = args->fd;
    struct sockaddr_in sock;
    memcpy(&sock,&(args->sock),sizeof(struct sockaddr_in));

    // Release the passed in args as they are no longer needed
    free(args);

    // Log that a new connection has been made
    printf("New conn\n");

    // Init rio
    Rio_readinitb(&rio, fd);

    // Request structure to store all the request info
    request* req = malloc(sizeof(request));

    // Connection & Proxy-Connection always need to be close
    strcpy(req->conn,"close");
    strcpy(req->proxyConn,"close");
    req->extraCount=0;

    // Parse the request then forward it to the server, then return
    // the server's response to the client
    parse(&rio,fd,req);
    forward(req,fd);

    // Connection is done for now so just close it.
    Close(fd);
    return NULL;
}

void parse(rio_t* rio, int connfd, request* req){
    int n;
    char* bufPtr;
    char* tok;

    // Request buffer
    char buf[MAXLINE], method[MAXLINE], path[MAXLINE], version[MAXLINE], tmp[MAXLINE];

    // Get the method, uri, and version
    if((n=Rio_readlineb(rio,buf,MAXLINE)) != 0){
            sscanf(buf,"%s %s %s",method,path,version);
            if(strcasecmp(method,"GET")){
                // TODO: Make this respond to user with error 501
                errorMsg("Error only support method: GET\n");
            } else{
                strcpy(req->method,method);
                strcpy(req->version,"HTTP/1.0");
                strcpy(req->path,strchr((strchr(path,'.')), '/'));
            }
    }

    // Read lines until there are no more
    while((n=Rio_readlineb(rio,buf,MAXLINE)) != 0){

        // Copy our buffer so we have a buffer to tokenize
        char bufCpy[MAXBUF];
        strcpy(bufCpy,buf);

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
            if(strlen(bufPtr)==0)
                strcpy(req->port,"80");
            else
                strcpy(req->port,bufPtr);
            if(strcmp(req->host,"")==0 || strcmp(req->port,"")==0){
                printf("Bad host:port %s:%s\n",req->host,req->port);
                exit(0);
            }

        } else if(strcmp(tok,"Proxy-Connection:")==0){ // If our token is Proxy-Conn
            // We don't need to do anything we are always sending close for this
        } else if(strcmp(tok,"Connection:")==0){ // If our token is Connection
            // We don't need to do anything we are always sending close for this
        } else if(strcmp(tok,"User-Agent:")==0){ // If our token is User-Agent
            // We don't need to do anything we are always sending user_agent_hdr for this
        } else{ // Otherwise we have an extra
            // Add the extra to the extras array
            strcpy(req->extras[req->extraCount++],buf);
            if(strcmp(buf,"\r\n")==0){
                break;
            }
        }
        // Clear our buffer out
        memset(buf,0,sizeof(buf));
    }
}

void forward(request* req, int clientfd){
    rio_t rio;
    int i,n,size,len;
    int total_size = 0;
    char body[MAXLINE],buf[MAXLINE],contentLength[MAXLINE],tmp[MAXLINE];
    char* tok;
    char* tmpPtr;

    memset(contentLength,0,sizeof(contentLength));

    // Build msg body
    sprintf(body,"%s %s %s\r\n",req->method,req->path,req->version);
    sprintf(body+strlen(body),"Host: %s",req->host);
    sprintf(body+strlen(body),":%s\r\n",req->port);
    sprintf(body+strlen(body),"%s", user_agent_hdr);
    sprintf(body+strlen(body),"Connection: %s\r\n", req->conn);
    sprintf(body+strlen(body),"Proxy-Connection: %s\r\n", req->proxyConn);

    for(i = 0; i < req->extraCount; i++)
            sprintf(body+strlen(body),"%s",req->extras[i]);

    // Print the request
    printf("--------------------------------------------\n");
    printf("Forward Request\n");
    printf("%s",body);
    printf("--------------------------------------------\n");

    int serverfd = Open_clientfd(req->host, req->port);
    Rio_readinitb(&rio,serverfd);

    Rio_writen(serverfd,body,strlen(body));

    printf("Forward Response\n");
    while((n=Rio_readlineb(&rio,buf,MAXLINE)) != 0){

        // Extract content type and length
        strcpy(tmp,buf);
        tok = strtok_r(tmp," ",&tmpPtr);
        // For some reason tiny uses 'Content-length' as opposed to 'Content-Length'
        // which is what the real sites use.
        if(strcmp(tok,"Content-Length:")==0 || strcmp(tok,"Content-length:")==0){
            strcpy(contentLength,tmpPtr);
            contentLength[strlen(contentLength)-2]='\0';
        }

        // Write buffer out to stdout and to the client
        if(strcmp(buf,"\r\n")!=0)
            printf("%s",buf);
        Rio_writen(clientfd,buf,strlen(buf));

        // If the buffer is '\r\n' then we are done reading headers
        if(strcmp(buf,"\r\n")==0){
            break;
        }

        // Clear the buffer after each header line
        memset(buf,0,sizeof(buf));
    }
    printf("--------------------------------------------\n\n");

    if(strlen(contentLength) > 0){
        // Get the content length as a integer
        len = atoi(contentLength);

        // While length is greater than 0, there is more data to
        // forward to the client
        while (len > 0){
            // If the number of len is larger than maxline then fill body with
            // data up to MAXLINE bytes. Otherwise we don't need MAXLINE bytes
            // so just fill up to len bytes.
            int readn = (len > MAXLINE) ? MAXLINE : len;
            // If size is not equal to what we wanted to read there was some
            // error so notify the user and exit this thread.
            if ((size = Rio_readnb(&rio, body, readn)) != readn){
                errorMsg("read from server error\n");
                exit(0);
            }

            // Increment the total size by the amount of bytes we read
            total_size += size;
            // Write the body out to the client
            Rio_writen(clientfd, body, size);
            // Decrement len by the amount of bytes we read
            len -= readn;
        }
    } else{
        while((size = Rio_readnb(&rio,body,sizeof(body))) != 0){
            Rio_writen(clientfd,body,size);
        }
    }

    // Done with the server so close the connection
    Close(serverfd);
}

void errorMsg(char* error){
    //TODO:  print out error to client
    //TODO:  print out error to stderr to keep track in server
}
