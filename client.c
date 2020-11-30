#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <string.h>

#define PORT 1234
#define MAXDATASIZE 100

char sendbuf[1024];
char recvbuf[1024];
char name[100];
int fd;
int board[9];

void usage(){
    printf("Change your name\t\t input: 1 New_name\n");
    printf("Show all players\t\t input: 2\n");
    printf("Invite player\t\t\t input: 3 your_name other_player_name\n");
    printf("Logout\t\t\t\t input: exit\n\n");
}

char alp(int a){
    if (a==0)
        return ' ';
    else if (a==1)
        return 'O';
    else 
        return 'X';
}

void print_board(int *board){
    printf("┌───┬───┬───┐        ┌───┬───┬───┐\n");
    printf("│ 0 │ 1 │ 2 │        │ %c │ %c │ %c │\n", alp(board[0]),alp(board[1]),alp(board[2]));
    printf("├───┼───┼───┤        ├───┼───┼───┤\n");
    printf("│ 3 │ 4 │ 5 │        │ %c │ %c │ %c │\n", alp(board[3]),alp(board[4]),alp(board[5]));
    printf("├───┼───┼───┤        ├───┼───┼───┤\n");
    printf("│ 6 │ 7 │ 8 │        │ %c │ %c │ %c │\n", alp(board[6]),alp(board[7]),alp(board[8]));
    printf("└───┴───┴───┘        └───┴───┴───┘\n");
}

int choose_user_turn(int *board){
    int i=0;
    int inviter=0, invitee=0;
    for(i=0; i<9; i++){
        if(board[i] == 1){
            inviter++;
        }
        else if(board[i] == 2){
            invitee++;
        }
    }
    if(inviter > invitee)
        return 2;
    else
        return 1;
}

void write_on_board(int *board, int location){
    int user_choice = choose_user_turn(board);
    board[location] = user_choice;
    sprintf(sendbuf, "7  %d %d %d %d %d %d %d %d %d\n", board[0], \
        board[1],board[2],board[3],board[4],board[5],board[6],board[7],board[8]);
}

void pthread_recv(void* ptr)
{
    int instruction;
    while(1)
    {
        memset(sendbuf,0,sizeof(sendbuf));
        instruction = 0;
        if ((recv(fd,recvbuf,MAXDATASIZE,0)) == -1)
        {
            printf("recv() error\n");
            exit(1);
        }
        sscanf (recvbuf,"%d",&instruction);
        switch (instruction)
        {
            case 2: {
                printf("%s\n", &recvbuf[2]); 
                break;
            }
            case 4: {
                char inviter[100];
                sscanf(recvbuf,"%d %s",&instruction, inviter);
                printf("%s\n", &recvbuf[2]);
                printf("If accept, input:5 1 %s\n", inviter);
                printf("If not, input:5 0 %s\n", inviter);
                break;
            }
            case 6: {
                printf("Start!\n");
                printf("Inviter go first.\n");
                printf("Please input the number between -0 and -8\n");
                print_board(board);
                break;
            }
            case 8: {
                // int board[9];
                char msg[100];
                sscanf (recvbuf,"%d %d%d%d%d%d%d%d%d%d %s",&instruction, \
                    &board[0],&board[1],&board[2],&board[3],&board[4],&board[5],&board[6], \
                        &board[7],&board[8], msg);
                print_board(board);
                printf("%s\n", msg);
		if(strstr(msg,"!")>0) {
			printf("\n");
			printf("New game\n");
			usage();
			memset(board,0,sizeof(board));
		}		 
		else printf("Please input the number between -0 and -8\n");
                break;
            }	
            
            default:
                break;
        }

        memset(recvbuf,0,sizeof(recvbuf));
    }
}



int main(int argc, char *argv[])
{
    int  numbytes;
    char buf[MAXDATASIZE];
    struct hostent *he;
    struct sockaddr_in server;


    if (argc !=2)
    {
        printf("Usage: %s <IP Address>\n",argv[0]);
        exit(1);
    }


    if ((he=gethostbyname(argv[1]))==NULL)
    {
        printf("gethostbyname() error\n");
        exit(1);
    }

    if ((fd=socket(AF_INET, SOCK_STREAM, 0))==-1)
    {
        printf("socket() error\n");
        exit(1);
    }

    bzero(&server,sizeof(server));

    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);
    server.sin_addr = *((struct in_addr *)he->h_addr);
    if(connect(fd, (struct sockaddr *)&server,sizeof(struct sockaddr))==-1)
    {
        printf("connect error\n");
        exit(1);
    }

    // First, Add User.
    printf("connected\n");
    char str[]=" have come in\n";
    printf("Pleace enter your name：");
    fgets(name,sizeof(name),stdin);
    name[strlen(name)-1] = '\0';
    char package[100];
    memset(package, '\0', sizeof(package));
    strcat(package, "1 ");
    strcat(package, name);
    send(fd, package, (strlen(package)),0);

    // usage
    usage();

    // Only handle message from server to client. (Goto pthread_recv finction)
    pthread_t tid;
    pthread_create(&tid, NULL, (void*)pthread_recv, NULL);
    // Only handle message from client to server.
    while(1){
        memset(sendbuf,0,sizeof(sendbuf));
        fgets(sendbuf,sizeof(sendbuf),stdin);   // Input instructions
        int location;
        if(sendbuf[0] == '-'){
            sscanf(&sendbuf[1], "%d", &location);
	    if(board[location] == 0) write_on_board(board, location);
	    else printf("try again ( you put on the same place )\n");
        }

        // logout
        if(strcmp(sendbuf,"exit\n")==0){          
            memset(sendbuf,0,sizeof(sendbuf));
            printf("quit.\n");
            return 0;
        }
        send(fd,sendbuf,(strlen(sendbuf)),0);   // Send instructions to server
    }
    pthread_join(tid,NULL);
    close(fd);
}
