#include "server.h"
#include "scanner.h"

#define PORT 12345
#define MAXLINE 1000

int listenfd = -1;
static struct sockaddr_in servaddr, cliaddr;
static socklen_t clientLen=0;
static char command[16] =  "";

pthread_t tidServer;

int is_blank(const char *s){
    while ((*s==' ')||(*s=='\t')||(*s=='\r')||(*s=='\n')){
        s++;
    }
    return *s=='\0';
}

void trim(char *s){
    size_t n =  strlen(s);
    if(n==0){return;}

    while (n > 0 && ((s[n-1] == '\r') ||( s[n-1] == '\n'))){
        --n;
        s[n] = '\0';
    }
}

int send_to_client(const void *buf, size_t len, const struct sockaddr *p, socklen_t pl){
    return sendto(listenfd, buf, len, 0, p, pl) == (ssize_t)len;
}

void server_init(void){
    bzero(&servaddr, sizeof(servaddr));

    // create a UDP Socket
    listenfd = socket(AF_INET, SOCK_DGRAM, 0);        
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(PORT);
    servaddr.sin_family = AF_INET; 
 
    // bind server address to socket descriptor
    bind(listenfd, (struct sockaddr*)&servaddr, sizeof(servaddr));
    pthread_create(&tidServer, NULL,runServer,0);
    return;
}

void* runServer(void *arg){
    (void) arg;
    int retnum=0;
    clientLen = sizeof(cliaddr);

    while(true){
        char buffer[1024];
        int n = recvfrom(listenfd, buffer, sizeof(buffer), 0, (struct sockaddr*)&cliaddr,&clientLen); //receive message from server

        buffer[n] = '\0';
        trim(buffer);
        //printf("%s",buffer);
        const char *cmd = buffer;
        char retmsg[12];
        //printf(cmd); //for testing

        if (!strcmp(cmd, "mode 0")){
            setMode(0);
            send_to_client("0", strlen("0"),(struct sockaddr *)&cliaddr, clientLen);
        }         
        else if (!strcmp(cmd, "mode 1")){
            setMode(1);
            send_to_client("1", strlen("1"),(struct sockaddr *)&cliaddr, clientLen);
        }         
        else if (!strcmp(cmd, "mode 2")){
            setMode(2);
            send_to_client("2", strlen("2"),(struct sockaddr *)&cliaddr, clientLen);
        }
        else if (!strcmp(cmd, "start")){
            setStartSignal(1);
            send_to_client("Started", strlen("Started"),(struct sockaddr *)&cliaddr, clientLen);
        }         
        else if (!strcmp(cmd, "pause toggle")){
            retnum=toggleIsPaused();
            snprintf(retmsg, sizeof(retmsg), "toggle %d",retnum);
            send_to_client(retmsg, strlen(retmsg),(struct sockaddr *)&cliaddr, clientLen);
        }         
        else if (!strcmp(cmd, "stop")){
            setIsStoped(true);
            send_to_client("stopped", strlen("stopped"),(struct sockaddr *)&cliaddr, clientLen);
        }
        else if (!strcmp(cmd, "shutdown")){
            sendShutdownSignal();
            const char *msg = "Program terminating.\n";
            send_to_client(msg, strlen(msg), (struct sockaddr *)&cliaddr, clientLen);
            break;

        }
        else if(cmd[0]=='c'){
            int token[3]={0};
            int i=0;

            char * myPtr = strtok((char*)cmd," ");
            myPtr=strtok(NULL," ");
            while(myPtr !=NULL){
                token[i]=atoi(myPtr);
                myPtr=strtok(NULL," ");
                i++;
            }

            setCustomSamplePerRev(token[0]);
            setCustomHeightChange(token[1]);
            setCustomNumOfHeights(token[2]);

            snprintf(retmsg, sizeof(retmsg), "%d %d %d",getCustomSamplePerRev(),getCustomHeightChange(),getCustomNumOfHeights());
            send_to_client(retmsg, strlen(retmsg),(struct sockaddr *)&cliaddr, clientLen);

        }
        else{send_to_client("NULL", strlen("NULL"),(struct sockaddr *)&cliaddr, clientLen);}
    }
    return NULL;
}

void server_cleanup(void)
{
    if (listenfd >= 0){
        int s = listenfd;
        listenfd = -1;
        close(s);
    }

    if (tidServer){
        (void)pthread_join(tidServer, NULL);
        tidServer = 0;
    }

    command[0] = '\0';
}