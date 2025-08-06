
#include "ECO_common.h"

#define NUM_MAX_CHESS_BOARDS (256)

//#define SERVER_ADDRESS "0.0.0.0"


//MUST CHANGE THIS HERE AND ON ECO_client.c TO WORK PROPERLY
int CHANGE_TURNS_ENABLED  = true;

int sockfd_array[FD_SETSIZE];
struct s_chessBoard gameBoards[NUM_MAX_CHESS_BOARDS];
int boardsInUse[NUM_MAX_CHESS_BOARDS] = {0};

//struct queue* MM_queue = NULL;

int matched_client1= -1;
int matched_client2= -1;

int openSockfdArraySpace(){
    int i = 1;
    //if a board is being used, check next board
    while(sockfd_array[i]){
        i++;
    }
    return i;
}

int openBoardSpace(){
    int i = 0;
    //if a board is being used, check next board
    while(boardsInUse[i]){
        i++;
    }
    return i;
}


void drawSocketFD(fd_set *sockets, int sockets_connected){
    printf("[");
    for(int i = 0; i < FD_SETSIZE;i++){
        if(FD_ISSET(i, sockets)){
            printf(" %d ", i);
        } 
    }
    printf("]\n");
}

void initializeBoard(int s_boardId,u_int64_t client1, u_int64_t client2){
    printf("IB: creating chess board\n");
    gameBoards[s_boardId].board.spaces[matrixSpaceAt(0,0)].residingPiece = PAWN;
    //printf("Piece at board 0, space 0,0: %s\n", PIECE_TO_STRING_TEXT(gameBoards[s_boardId].board.spaces[matrixSpaceAt(0,0)].residingPiece));
    //server_board->board = malloc(sizeof(struct chess_board));
    chess_boardCreate(&gameBoards[s_boardId].board);
    gameBoards[s_boardId].clientWhiteId = client1;
    gameBoards[s_boardId].clientBlackId = client2;
    boardsInUse[s_boardId] = true;
}

void requestToStartGame(int c_socket){
    struct response server_response;
    
    if(matched_client1 == -1){
            printf("SG: Client %d is waiting to start a game\n", c_socket);
            matched_client1 = c_socket;
            return;
        }
    else{
        matched_client2 = c_socket;
        int board_id = openBoardSpace();
        int player_black, player_white;
        //randomly assign matched clients to white and black
        //int randomNum = rand();
        //printf("SG: Random Number Generated: %d\n", randomNum%2);
        if(rand()%2){
            player_white = matched_client1;
            player_black = matched_client2;
        }
        else{
            player_black = matched_client1;
            player_white = matched_client2;
        }

        printf("SG: initializing Board \n");
        initializeBoard(board_id, player_white, player_black);

        server_response.client_id = player_white;
        server_response.board_id = board_id;
        server_response.sc_comm = GAME_START_WHITE;
        
        if(send(player_white, (char*)&server_response, sizeof(struct response),0)<0){
            perror("SG: Failed to Send start Game for White\n");
            exit(EXIT_FAILURE);
        }

        server_response.sc_comm = GAME_START_BLACK;
        if(send(player_black, (char*)&server_response, sizeof(struct response),0)<0){
            perror("SG: Failed to Send start Game for Black\n");
            exit(EXIT_FAILURE);
        }
        printf("SG: Board Initialize Success\n");
        printf("SG: Clients %d and %d are now matched together\n",matched_client1, matched_client2);
        matched_client1 = -1;
        matched_client2 = -1;
        return;
    }
}

void requestToMove(int client_socket, struct request* moveRequest){
    struct response server_response;
    enum moveErr moveOutput = chessServer_move(&gameBoards[moveRequest->board_id].board, moveRequest->move_req.startRow,
                        moveRequest->move_req.startCol, moveRequest->move_req.endRow, moveRequest->move_req.endCol);
        if (moveOutput == VALID_PLACEMENT){
            
            //response to player moving piece
            server_response.sc_comm = VALID_PLACEMENT;
            if(send(client_socket, (char*)&server_response, sizeof(struct response),0)<0){
                    perror("Error Sending Confirmation of Move\n");
            }
            if(CHANGE_TURNS_ENABLED ){
                change_turn(&gameBoards[moveRequest->board_id].board);
            }
            //response to other player notifying them a move has been made and its thier turn
            memcpy(&server_response.opp_move, &moveRequest->move_req, sizeof(struct movement));
            if (client_socket == gameBoards[moveRequest->board_id].clientWhiteId){
                 if(send(gameBoards[moveRequest->board_id].clientBlackId, (char*)&server_response, sizeof(struct response),0)<0){
                    perror("Error Sending Move to Opponent\n");
                }               
            }
            else{
                if(send(gameBoards[moveRequest->board_id].clientWhiteId, (char*)&server_response, sizeof(struct response),0)<0){
                    perror("Error Sending Move to Opponent\n");
                }
            }
        }
        
        else if (moveOutput == WINNING_MOVE){
            server_response.sc_comm = WINNING_MOVE;
            if(gameBoards[moveRequest->board_id].board.board_turn == WHITE){
                if(send(gameBoards[moveRequest->board_id].clientWhiteId, (char*)&server_response, sizeof(struct response),0)<0){
                perror("Error Sending Confirmation of Move\n");
                }

                //might cause problem bc of the derefferencing of movement struct 
                server_response.opp_move = moveRequest->move_req;
                if(send(gameBoards[moveRequest->board_id].clientBlackId, (char*)&server_response, sizeof(struct response),0)<0){
                perror("Error Sending Confirmation of Move\n");
                }
            }
            else{
                if(send(gameBoards[moveRequest->board_id].clientBlackId, (char*)&server_response, sizeof(struct response),0)<0){
                perror("Error Sending Confirmation of Move\n");
                }

                //might cause problem bc of the derefferencing of movement struct 
                server_response.opp_move = moveRequest->move_req;
                if(send(gameBoards[moveRequest->board_id].clientWhiteId, (char*)&server_response, sizeof(struct response),0)<0){
                perror("Error Sending Confirmation of Move\n");
                }
            }
            boardsInUse[moveRequest->board_id] = false;

        }
        else{
            server_response.sc_comm = moveOutput;
            if(send(client_socket, (char*)&server_response, sizeof(struct response),0)<0){
                perror("Error Sending Confirmation of Move\n");
            }
        }
}

int handle_request(int c_socket){
    struct request client_request;

    printf("HR: Waiting to recieve request...");
    if(recv(c_socket, (char*)&client_request, sizeof(struct request),0) < 0){
        printf("Connnection Lost\n");
        return 1;
    }
    printf("package recieved\n");
    printf("HR: Verifying packet: client id %ld ", client_request.client_id);
    printf("board id: %ld", client_request.board_id);
    printf("\n");
    printf("HR: %s\n", MOVEERR_TO_STRING(client_request.sc_comm));
    switch(client_request.sc_comm){
        case START_GAME_REQUEST:
        requestToStartGame(c_socket);
        break;

        case MOVE_REQUEST:
        requestToMove(c_socket, &client_request);
        break;
    }
    return 0;
}

void handle_connection(int s_socket){
    //MM_queue = malloc(sizeof(struct queue));
    //queueInitialize(MM_queue);

    //select() that checks connected sockets and 
    //current_sockets is main fd, and ready_sockets we use to save socket we
    //are currently addrtessing, b/c select is destructive like ram
    fd_set current_sockets, ready_sockets;
    int sockets_connected = 1;

    //current_sockets is bitfield, this sets it all to zero
    FD_ZERO(&current_sockets);

    //add the main(server) socket to current sockets 
    printf("HC: Server socket input to HC %d\n", s_socket);
    
    FD_SET(s_socket, &current_sockets);
    printf("HC: Server Socket: %ld with count: %d\n", current_sockets.__fds_bits[0], sockets_connected - 1);

    int maxConnSocket = 1;

    printf("HC: Server Socket Set as: % d\n", s_socket);
    printf("fd set size max: %d\n", FD_SETSIZE);

    printf("HC: Server Ready to Connect to Clients\n");

    while(true){
        printf("\nHC: Listening for Activity Sockets: \n");
        printf("HC: sockets_Connected: %d \n", sockets_connected);

        drawSocketFD(&current_sockets, 10);
        printf("HC: Number of Clients Currently Connected to the Server: %d\n", sockets_connected - 1);
        ready_sockets = current_sockets;

        //printf("HC: Current Server Socket: %d with count: %d\n", ready_sockets.fd_array[0], ready_sockets.fd_count);

        //fd_setsize is max number allowed in set size, defined as 64 in our system
        //         maxConnSocket
        if (select(FD_SETSIZE, &ready_sockets, NULL, NULL, NULL ) < 0){
            perror("HC: Select Failed\n");
            exit(EXIT_FAILURE);
        }

        for (int i=0; i < FD_SETSIZE; i++){
            if(FD_ISSET(i, &ready_sockets)){
                if (i == s_socket){
                    printf("HC: Client attempt to connect...");
                    int client_socket = accept(s_socket, NULL, NULL);
                    FD_SET(client_socket, &current_sockets);
                    FD_CLR(s_socket, &ready_sockets);
                    maxConnSocket++;
                    printf("Client %d Accepted to the Server\n", client_socket);
                    sockets_connected++;
                }
                else{
                    //printf("HC: Current Server Socket: %d \n", ready_sockets.fd_array[0]);
                    printf("HC: Handling Request...\n");
                    if(handle_request(i)){
                        printf("HC: Client Lost Connection to Server");
                        FD_CLR(i, &current_sockets);
                        sockets_connected--;
                    }
                    printf("HC: Handle Request Success\n");
                }
            }
        }
        printf("HC: Activity on Socket: ");
        //drawSocketFD(ready_sockets, sockets_connected);
        
    }
    


}

int main(int argc, char *argv[]){
    titleServerECO_draw();
    int sockfd, socket_port, accepted_client, opt;
    struct sockaddr_in addr;
    //setting time as seed for rand()
    srand(time(0));
    
    while((opt = getopt(argc,argv, "c")) != -1){
		switch (opt){
			case 'c' :
				CHANGE_TURNS_ENABLED  = true;
				printf("INFO: Disabling Changing turns after a player moves\n");
				break;
		}
	}

    //set port from command line
    if (argc > 1) {
		socket_port = strtol(argv[1], NULL, 10);
		printf("INFO: setting server port as: %d\n", socket_port);
	} else {
		//fprintf(stderr, USAGE_STRING, argv[0]);
        socket_port = SERVER_PORT;
        printf("INFO: setting server port as: %d\n", socket_port);

	}

	printf("Initialised.\n");

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1){
        //add error check info
        printf("Could not create socket error num: %d", errno);
    }
    printf("Socket Created: %d\n", sockfd);

    int optval = 1;
	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (void *)&optval, sizeof(optval));

    //initialize address & port for the server
    struct sockaddr_in ECOserver_address;
    ECOserver_address.sin_family = AF_INET;
    ECOserver_address.sin_port = htons(SERVER_PORT);

    //BETTER SOULTION FOR SEVER ADDR
    //"" ALLOWS SERVER TO JUST USE ITS OWN ADDRESS
    //ECOserver_address.sin_addr.s_addr = inet_addr(SERVER_ADDRESS);
    ECOserver_address.sin_addr.s_addr = INADDR_ANY;

    printf("Set server IP as: %d\n", ECOserver_address.sin_addr.s_addr);

    //bind address & port to our socket
    if(bind(sockfd, (struct sockaddr *)&ECOserver_address,sizeof(ECOserver_address)) < 0){
		printf("%d\n", errno);
        perror("Unable to bind socket");
        
		return EXIT_FAILURE;
    }

    //listen for connections from client/s
    if(listen(sockfd, 5) < 0){
		perror("Unable to listen on socket");
		return EXIT_FAILURE;
    }
    
    //int client_socket = accept(sockfd, NULL, NULL);
    //printf("Accepted client socket: %d\n", client_socket);
    
    handle_connection(sockfd);

    close(sockfd);

}
