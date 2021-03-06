/*********************************************
*
* Authors: 	Matthew Rodgers 					G00847854
*         			 Jimmy D. Bodden Pineda 	G00931220
*
*********************************************/


#include <stdio.h>
#include <string.h>
#include "csapp.h"
#include "list.h"

// TODO: Address errno being 0. Make sure that you're using
// TODO: it in the right places. Check csapp.c cor places that set it.

typedef struct _request {
    char method[16];
    char path[MAXLINE];
    char version[16];
    char host[MAXLINE];
    char port[MAXLINE];
    char conn[6];
    char proxyConn[6];
    char extras[20][MAXLINE];
    int extraCount;
} request;

typedef struct _threadArgs {
    int fd;
    struct sockaddr_in sock;
    socklen_t clientlen;
} threadArgs;

int parse(rio_t *, int, request *);

void forward(request *, int);

void *thread(void *argv);

void errorMsg(char *error, int, rio_t *);

void serverError(int, rio_t *);

int isLocal(char *path);

int hasPort(char *path);

//My function prototypes
int cacheIsFull();

int fitsInCache(int size);

int fitsInNode(int size, node_ptr node);

void giveBackCachedData(char *data, int fd);

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";

//Shared Cache=========================

//Initialize size of cache
int cacheSize;

//Create the cache list
node_ptr cache;

//Shared Cache=========================

// Semaphores
//Synchronization Methods
sem_t reading;
sem_t writing;

pthread_mutex_t mutex;

//My cache functions------------------------------
int cacheIsFull() {
    return cacheSize == MAX_CACHE_SIZE;
}

int fitsInCache(int size) {
    int newSize = cacheSize + size;

    if (newSize > MAX_CACHE_SIZE)
        return 0;
    else
        return 1;
}

int fitsInNode(int size, node_ptr node)
{
    if(!node)
        return 0;

    int newSize = node->size+size;

    if(newSize > MAX_OBJECT_SIZE)
    {
        return 0;
    }
    else
    {
        return 1;
    }
}

void giveBackCachedData(char *data, int fd) {

}

//My cache functions------------------------------


void sigpipe_handler(int sig) {
    printf("SIGPIPE received and ignored\n");
    return;
}

int main(int argc, char **argv) {
    //init cache
    cache = makeSingleList();
    cacheSize = 0;

    //init semaphores
    sem_init(&reading, 0, 0);
    sem_init(&writing, 0, 0);

    //mutex
    pthread_mutex_init(&mutex, 0);

    int listenfd, connfd;
    socklen_t clientlen;
    struct sockaddr_in clientAddr;
    pthread_t tid;

    // Install SIGPIPE handler
    Signal(SIGPIPE, sigpipe_handler);

    // If we have a port passed in continue
    if (argc == 2) {
        // Open socket to listen on
        if ((listenfd = open_listenfd(argv[1])) < 0) {
            printf("Server Error: %d %s", errno, strerror(errno));
        } else {
            // Notify user that proxy is running
            printf("Proxy running on port: %s\n", argv[1]);

            // Listen loop
            while (1) {
                // Create a socket to read from
                clientlen = sizeof(clientAddr);

                // Initialize thread args to pass to the new thread
                threadArgs *args = (threadArgs *) malloc(sizeof(threadArgs));
                // Block until we get a connection
                connfd = Accept(listenfd, (SA *) (&(args->sock)), &clientlen);

                if (args != NULL) {

                    // Add the new socket to the thread args and create a new thread
                    // to handle the connection
                    args->fd = connfd;
                    Pthread_create(&tid, NULL, thread, (void *) args);
                } else {
                    rio_t rio;
                    Rio_readinitb(&rio, connfd);
                    errorMsg("Server Error: 500", connfd, &rio);
                }
            }


            exit(0);
        }
    }
	
	//Delete List
	if(cache)
		deleteList(cache);

    sem_destroy(&reading);
    sem_destroy(&writing);

    return 0;
}

void *thread(void *argv) {
    rio_t rio;

    // Detach the thread from the main thread so the main
    // thread doesn't need to handle resources after this
    // thread exits
    Pthread_detach(pthread_self());

    // Install SIGPIPE handler
    Signal(SIGPIPE, sigpipe_handler);

    // Convert passed in arguments to a threadArgs structure
    // So we can extract the passed in thread arguments.
    threadArgs *args = (threadArgs *) argv;
    int fd = args->fd;
    struct sockaddr_in sock;
    memcpy(&sock, &(args->sock), sizeof(struct sockaddr_in));

    // Release the passed in args as they are no longer needed
    free(args);

    // Init rio
    Rio_readinitb(&rio, fd);

    // Request structure to store all the request info
    request *req = malloc(sizeof(request));
    if (req != NULL) {
        // Connection & Proxy-Connection always need to be close
        strcpy(req->conn, "close");
        strcpy(req->proxyConn, "close");
        req->extraCount = 0;

        // Parse the request then forward it to the server, then return
        // the server's response to the client
        if (parse(&rio, fd, req) == 1) {

            //Check to see if the request has been done before
            //selectNode will look for the node that has the same
            //method as the one from the current request.
            //Returns NULL if there were no previous requests
            node_ptr theNode;

            //ADD SOME KIND OF SYNCHRONIZATION HERE
            //sem_wait(&writing);
            //pthread_mutex_lock(&mutex);

            theNode = selectNodeByPath(req->port, req->host, req->path, cache);

            //pthread_mutex_unlock(&mutex);
            //sem_post(&reading);
            //END SOME KIND OF SYNCHRONIZATION HERE

            if (theNode) {

                //print out the forwarded response
                char body[MAXLINE];
                // Build msg body
                sprintf(body, "%s %s %s\r\n", req->method, req->path, req->version);
                sprintf(body + strlen(body), "Host: %s", req->host);
                sprintf(body + strlen(body), ":%s\r\n", req->port);
                sprintf(body + strlen(body), "%s", user_agent_hdr);
                sprintf(body + strlen(body), "Connection: %s\r\n", req->conn);
                sprintf(body + strlen(body), "Proxy-Connection: %s\r\n", req->proxyConn);

                int i;
                for (i = 0; i < req->extraCount; i++)
                    sprintf(body + strlen(body), "%s", req->extras[i]);

                // Print the request
                printf("--------------------------------------------\n");
                printf("Fetching Cached Request\n");
                printf("%s", body);
                printf("--------------------------------------------\n");

                incrementCountForNode(theNode);

                //Write request directly to client from node
                rio_writen(fd, theNode->data, theNode->size);

            } else {
                //Forward the request.
                //Forward will be in charge of putting new objects in the cache
                forward(req, fd);
            }

            //NO CACHING VERSION
//            forward(req, fd);
        }
    } else {
        errorMsg("Server Error", fd, &rio);
    }
    // Connection is done for now so just close it.
    if (Close(fd) < 0) {
        printf("Server Error: %d %s", errno, strerror(errno));
    }
    return NULL;
}

int parse(rio_t *rio, int connfd, request *req) {
    int n;
    char *bufPtr;
    char *tok;

    // Request buffer
    char buf[MAXLINE], method[MAXLINE], path[MAXLINE], version[MAXLINE], tmp[MAXLINE];

    // Get the method, uri, and version
    if ((n = Rio_readlineb(rio, buf, MAXLINE)) != 0) {
        sscanf(buf, "%s %s %s", method, path, version);
        if (strcasecmp(method, "GET")) {
            char msg[200];
            sprintf(msg, "Server Error: %d %s\n", 501, "Supported methods: GET");
            errorMsg(msg, connfd, rio);
        } else {
            strcpy(req->method, method);
            strcpy(req->version, "HTTP/1.0");

            tok = strtok_r(path, "/", &bufPtr);
            if (strcmp(tok, "http:") == 0)
                bufPtr++;
            if (isLocal(bufPtr)) {
                if (hasPort(bufPtr)) {
                    tok = strtok_r(bufPtr, ":", &bufPtr);
                    strcpy(req->path, strchr(bufPtr, '/'));
                } else {
                    tok = strtok_r(bufPtr, "/", &bufPtr);
                    if (strcmp(bufPtr, "") == 0)
                        strcpy(req->path, "/");
                    else {
                        strcat(req->path, "/");
                        strcpy(req->path + strlen(req->path), bufPtr);
                    }
                }
            } else {
                strcpy(req->path, strchr((strchr(bufPtr, '.')), '/'));
            }
        }
    }

    if (strlen(req->path) != 0) {
        // Read lines until there are no more
        while ((n = Rio_readlineb(rio, buf, MAXLINE)) != 0) {

            // Copy our buffer so we have a buffer to tokenize
            char bufCpy[MAXBUF];
            strcpy(bufCpy, buf);

            // Get the first token
            tok = strtok_r(bufCpy, " ", &bufPtr);

            if (strcmp(tok, "Host:") == 0) { // If our token is Host
                // Get the value of Host and store it in the request host
                tok = strtok_r(bufPtr, " ", &bufPtr);
                tok[strcspn(tok, "\r\n")] = 0;
                strcpy(req->host, tok);
                strcpy(tmp, req->host);
                tok = strtok_r(tmp, ":", &bufPtr);
                strcpy(req->host, tok);
                if (strlen(bufPtr) == 0)
                    strcpy(req->port, "80");
                else
                    strcpy(req->port, bufPtr);
                if (strcmp(req->host, "") == 0 || strcmp(req->port, "") == 0) {
                    printf("Bad host:port %s:%s\n", req->host, req->port);
                    exit(0);
                }

            } else if (strcmp(tok, "Proxy-Connection:") == 0) { // If our token is Proxy-Conn
                // We don't need to do anything we are always sending close for this
            } else if (strcmp(tok, "Connection:") == 0) { // If our token is Connection
                // We don't need to do anything we are always sending close for this
            } else if (strcmp(tok, "User-Agent:") == 0) { // If our token is User-Agent
                // We don't need to do anything we are always sending user_agent_hdr for this
            } else { // Otherwise we have an extra
                // Add the extra to the extras array
                strcpy(req->extras[req->extraCount++], buf);
                if (strcmp(buf, "\r\n") == 0) {
                    break;
                }
            }
            // Clear our buffer out
            memset(buf, 0, sizeof(buf));
        }
        return 1;
    }
    return -1;

}

void forward(request *req, int clientfd) {

    //Make node that will be put into cache
    node_ptr node = makeNode(req->port, req->host, req->path, NULL, 0, 1, NULL, NULL);

    rio_t rio;
    int i, n, size, len;
    int total_size = 0;
    char body[MAXLINE], buf[MAXLINE], contentLength[MAXLINE], tmp[MAXLINE];
    char *tok;
    char *tmpPtr;

    memset(contentLength, 0, sizeof(contentLength));

    // Build msg body
    sprintf(body, "%s %s %s\r\n", req->method, req->path, req->version);
    sprintf(body + strlen(body), "Host: %s", req->host);
    sprintf(body + strlen(body), ":%s\r\n", req->port);
    sprintf(body + strlen(body), "%s", user_agent_hdr);
    sprintf(body + strlen(body), "Connection: %s\r\n", req->conn);
    sprintf(body + strlen(body), "Proxy-Connection: %s\r\n", req->proxyConn);

    for (i = 0; i < req->extraCount; i++)
        sprintf(body + strlen(body), "%s", req->extras[i]);

    // Print the request
    printf("--------------------------------------------\n");
    printf("Forward Request\n");
    printf("%s", body);
    printf("--------------------------------------------\n");

    int serverfd;
    if ((serverfd = open_clientfd(req->host, req->port)) < 0) {
        rio_t clientRio;
        Rio_readinitb(&clientRio, clientfd);
        char msg[200];
        sprintf(msg, "Server Error: %d %s\n", errno, strerror(errno));
        errorMsg(msg, clientfd, &clientRio);
    } else {
        Rio_readinitb(&rio, serverfd);

        if ((n = rio_writen(serverfd, body, strlen(body))) < 0) {
            if (n != -1) {
                rio_t clientRio;
                Rio_readinitb(&clientRio, clientfd);
                char msg[200];
                sprintf(msg, "Server Error: %d %s\n", errno, strerror(errno));
                errorMsg(msg, clientfd, &clientRio);
            }
        } else {

            printf("Forward Response\n");
            while ((n = Rio_readlineb(&rio, buf, MAXLINE)) != 0) {

                // Extract content type and length
                strcpy(tmp, buf);
                tok = strtok_r(tmp, " ", &tmpPtr);
                // For some reason tiny uses 'Content-length' as opposed to 'Content-Length'
                // which is what the real sites use.
                if (strcmp(tok, "Content-Length:") == 0 || strcmp(tok, "Content-length:") == 0) {
                    strcpy(contentLength, tmpPtr);
                    contentLength[strlen(contentLength) - 2] = '\0';
                }

                // Write buffer out to stdout and to the client
                if (strcmp(buf, "\r\n") != 0)
                    printf("%s", buf);
                n = rio_writen(clientfd, buf, strlen(buf));
                if (n == -1)
                    break;

                if (strcmp(buf, "\r\n") == 0) {
                    break;
                }

                // Clear the buffer after each header line
                memset(buf, 0, sizeof(buf));

            }

            printf("--------------------------------------------\n\n");

            if (strlen(contentLength) > 0) {
                // Get the content length as a integer
                len = atoi(contentLength);

                // While length is greater than 0, there is more data to
                // forward to the client

                while (len > 0) {
                    // If the number of len is larger than maxline then fill body with
                    // data up to MAXLINE bytes. Otherwise we don't need MAXLINE bytes
                    // so just fill up to len bytes.
                    int readn = (len > MAXLINE) ? MAXLINE : len;
                    // If size is not equal to what we wanted to read there was some
                    // error so notify the user and exit this thread.
                    if ((size = Rio_readnb(&rio, body, readn)) != readn) {
                        rio_t clientRio;
                        Rio_readinitb(&clientRio, clientfd);
                        char msg[200];
                        sprintf(msg, "Server Error: %d %s\n", errno, strerror(errno));
                        errorMsg(msg, clientfd, &clientRio);
                        break;
                    } else {

                        //Chunk was read, check if chunk fits in node---------------------------
                        if(fitsInNode(size, node))
                        {
                            //concatenate bytes into node
                            memcpy(node->data+node->size, body, size);

							 //if fits, increment node size and
                            node->size+=size;
                        }
                        else
                        {
                            //object doesn't fit, don't cache.
                            //dump node and check for NULL later to not insert
                            if(node)
                                freeNode(node);

							node = NULL;
                        }
                        //------------------------------------------------------------------------

                        // Increment the total size by the amount of bytes we read
                        total_size += size;
                        // Write the body out to the client
                        if ((n = rio_writen(clientfd, body, size)) < 0) {
                            if (n == -1)
                                break;
                            rio_t clientRio;
                            Rio_readinitb(&clientRio, clientfd);
                            char msg[200];
                            sprintf(msg, "Server Error: %d %s\n", errno, strerror(errno));
                            errorMsg(msg, clientfd, &clientRio);
                        }
                        // Decrement len by the amount of bytes we read
                        len -= readn;
                    }

                }
            } else {
                while ((size = Rio_readnb(&rio, body, sizeof(body))) != 0) {
                    // read failed
                    if (size == -1) {
                        rio_t clientRio;
                        Rio_readinitb(&clientRio, clientfd);
                        char msg[200];
                        sprintf(msg, "%d %s\n", errno, strerror(errno));
                        errorMsg(msg, clientfd, &clientRio);
                    }

                    //Chunk was read, check if chunk fits in node---------------------------
                    if(fitsInNode(size, node))
                    {
                        //concatenate bytes into node
                        memcpy(node->data+node->size, body, size);

 						//if fits, increment node size and
                        node->size+=size;
                    }
                    else
                    {
                        //object doesn't fit, don't cache.
                        //dump node and check for NULL later to not insert
                        if(node)
                            freeNode(node);
                        node = NULL;
                    }
                    //------------------------------------------------------------------------

                    n = rio_writen(clientfd, body, size);



                    if (n == 1)
                        break;
                }
            }
        }

        //After info was read, insert node into cache.
        //This will check for NULL nodes. Won't insert NULL nodes.
        printf("INSERTING INTO LIST\n");
        fflush(stdout);

		if(fitsInCache(node->size)) {

            //sem_wait(&reading);
            //pthread_mutex_lock(&mutex);

            insertNode(node, cache);

            //pthread_mutex_unlock(&mutex);
            //sem_post(&writing);
        }
        else
        {
            //sem_wait(&reading);
            //pthread_mutex_lock(&mutex);

            node_ptr removed = removeByLRU(cache);

            if(removed)
                freeNode(removed);

            removed = NULL;
            insertNode(node, cache);

            //pthread_mutex_unlock(&mutex);
            //sem_post(&writing);

        }

        //sem_wait(&writing);
        //pthread_mutex_lock(&mutex);

        traverseListRight(cache);

        //pthread_mutex_unlock(&mutex);
        //sem_post(&reading);

        // Done with the server so close the connection
        if (close(serverfd) < 0) {
            printf("Server Error: %d %s", errno, strerror(errno));
        }
    }
}

void errorMsg(char *error, int fd, rio_t *rio) {
    printf("Error: %s\n", error);

    char header[MAXBUF];
    char body[MAXBUF];
    char res[MAXBUF];

    sprintf(body + strlen(body), "<html>\r\n");
    sprintf(body + strlen(body), "<head><title>Proxy Error</title><head>\r\n");
    sprintf(body + strlen(body), "<body bgcolor='ffffff'>%s</body></html>\r\n\r\n", error);

    sprintf(header, "HTTP/1.0 500\r\n");
    sprintf(header + strlen(header), "Content-Type: text/html; charset=iso-8859-1\r\n");
    sprintf(header + strlen(header), "Content-Length: %d\r\n", (int) strlen(body));
    sprintf(header + strlen(header), "Connection: close\r\n");
    sprintf(header + strlen(header), "\r\n");

    sprintf(res, "%s", header);
    sprintf(res + strlen(res), "%s", body);

    rio_writen(fd, res, strlen(res));
}

int isLocal(char *path) {
    char *tok;
    int i, len;
    char pathCopy[strlen(path) + 1];
    strcpy(pathCopy, path);
    tok = strtok(pathCopy, "/");
    len = strlen(tok);
    for (i = 0; i < len; i++) {
        if (*tok == '.')
            return 0;
        else
            tok++;
    }
    return 1;
}

int hasPort(char *path) {
    char *tok;
    int i, len;
    char pathCopy[strlen(path) + 1];
    strcpy(pathCopy, path);
    tok = strtok(pathCopy, "/");
    len = strlen(tok);
    for (i = 0; i < len; i++) {
        if (*tok == ':')
            return 1;
        else
            tok++;
    }
    return 0;
}
