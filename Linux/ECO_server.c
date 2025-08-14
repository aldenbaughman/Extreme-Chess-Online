
#include "ECO_common.h"

#define NUM_MAX_CHESS_BOARDS (256)

//#define SERVER_ADDRESS "0.0.0.0"


//MUST CHANGE THIS HERE AND ON ECO_client.c TO WORK PROPERLY
int CHANGE_TURNS_ENABLED  = true;

int sockfd_array[FD_SETSIZE];
struct s_chessBoard gameBoards[NUM_MAX_CHESS_BOARDS];
int boardsInUse[NUM_MAX_CHESS_BOARDS] = {0};

struct clientInfo connectedClientInfo[FD_SETSIZE];

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
    printf("[initializeBoard] Creating Chess Board\n");
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
    char serverPayload[BUFFER_SIZE];
    
    if(matched_client1 == -1){
            printf("[requestToStartGame] Client %d is waiting to start a game\n", c_socket);
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

        initializeBoard(board_id, player_white, player_black);

        server_response.client_id = player_white;
        server_response.board_id = board_id;
        server_response.sc_comm = GAME_START_WHITE;
        server_response.move.startRow = 0;
        server_response.move.startCol = 0;
        server_response.move.endRow = 0;
        server_response.move.endCol = 0;


        responseToPayload(serverPayload, server_response);

        printf("[requestToStartGame] Sending Start Game Messages to Matched Clients\n");
        
        if(send(player_white, serverPayload, sizeof(serverPayload),0)<0){
            perror("[requestToStartGame] Failed to Send start Game for White\n");
            exit(EXIT_FAILURE);
        }

        connectedClientInfo[player_white].clientOpponentsFD = player_black;
        connectedClientInfo[player_white].clientBoardID = board_id;
        connectedClientInfo[player_white].isOpponentConnected = true;

        printf("[requestToStartGame] Storing information about client %d: Opponent FD - %d   Board ID - %d isOpponentConnected - %d\n", player_white, 
                                                                                                                                        connectedClientInfo[player_white].clientOpponentsFD,
                                                                                                                                        connectedClientInfo[player_white].clientBoardID, 
                                                                                                                                        connectedClientInfo[player_white].isOpponentConnected = true);


        server_response.client_id = player_black;
        server_response.sc_comm = GAME_START_BLACK;

        responseToPayload(serverPayload, server_response);

        if(send(player_black, serverPayload, sizeof(serverPayload),0)<0){
            perror("[requestToStartGame] Failed to Send start Game for Black\n");
            exit(EXIT_FAILURE);
        }

        connectedClientInfo[player_black].clientOpponentsFD = player_white;
        connectedClientInfo[player_black].clientBoardID = board_id;
        connectedClientInfo[player_black].isOpponentConnected = true;

        printf("[requestToStartGame] Storing information about client %d: Opponent FD - %d   Board ID - %d isOpponentConnected - %d\n", player_black, 
                                                                                                                                        connectedClientInfo[player_black].clientOpponentsFD,
                                                                                                                                        connectedClientInfo[player_black].clientBoardID, 
                                                                                                                                        connectedClientInfo[player_black].isOpponentConnected = true);

        printf("[requestToStartGame] Clients %d and %d are now matched together\n",matched_client1, matched_client2);
        matched_client1 = -1;
        matched_client2 = -1;
        return;
    }
}

void requestToMove(int client_socket, struct response* moveRequest){
    struct response server_response;
    char serverResponse[BUFFER_SIZE];

    server_response.client_id = 0;
    server_response.sc_comm = 0;
    server_response.board_id = 0;
    server_response.move.startRow = 0;
    server_response.move.startCol = 0;
    server_response.move.endRow = 0;
    server_response.move.endCol = 0;

    printf("[requestToMove] Checking Client's Requested Move\n");
    enum moveErr moveOutput = chessServer_move(&gameBoards[moveRequest->board_id].board, 
                                                moveRequest->move.startRow,
                                                moveRequest->move.startCol, 
                                                moveRequest->move.endRow, 
                                                moveRequest->move.endCol);
        
    printf("[requestToMove] Clients Move is deemed: %s\n", MOVEERR_TO_STRING(moveOutput));
    
    if (!connectedClientInfo[client_socket].isOpponentConnected){
        //notify client that they have won through opp's forfit
        char notifyClientOfForfit[] = "0 14 0 0 0 0 0";
        if(send(client_socket, notifyClientOfForfit, sizeof(notifyClientOfForfit), 0) < 0){
            perror("[handle_connection] Error Notifing Opponent they won through forfit\n");
        }
        return;
    }
    if (moveOutput == VALID_PLACEMENT){
    
    //response to player moving piece
    server_response.sc_comm = VALID_PLACEMENT;
    memcpy(&server_response.move, &moveRequest->move, sizeof(struct movement));

    responseToPayload(serverResponse, server_response);

    printf("[requestToMove] Sending Response to Client: ");
    printf(" %ld", server_response.client_id);
    printf(" %d", server_response.sc_comm);
    printf(" %ld", server_response.board_id);
    printf(" %d", server_response.move.startRow);
    printf(" %d", server_response.move.startCol);
    printf(" %d", server_response.move.endRow);
    printf(" %d\n", server_response.move.endCol);

    if(send(client_socket, serverResponse, sizeof(serverResponse),0)<0){
            perror("[requestToMove] Error Sending Confirmation of Move\n");
    }
    if(CHANGE_TURNS_ENABLED ){
        change_turn(&gameBoards[moveRequest->board_id].board);
    }
    //response to other player notifying them a move has been made and its thier turn

    //memcpy(&server_response.opp_move, &moveRequest->move_req, sizeof(struct movement));
    printf("[requestToMove] Sending Client's move to Client's Oppenent\n");
    if (client_socket == gameBoards[moveRequest->board_id].clientWhiteId){
        if(send(gameBoards[moveRequest->board_id].clientBlackId, serverResponse, sizeof(serverResponse), 0)<0){
            perror("[requestToMove] Error Sending Move to Opponent\n");
        }               
    }
    else{
        if(send(gameBoards[moveRequest->board_id].clientWhiteId, serverResponse, sizeof(serverResponse),0)<0){
            perror("[requestToMove] Error Sending Move to Opponent\n");
        }
    }
}
    
    else if (moveOutput == WINNING_MOVE){
        server_response.sc_comm = WINNING_MOVE;
        responseToPayload(serverResponse, server_response);

        if(gameBoards[moveRequest->board_id].board.board_turn == WHITE){
            if(send(gameBoards[moveRequest->board_id].clientWhiteId, serverResponse, sizeof(serverResponse),0)<0){
            perror("[requestToMove] Error Sending Confirmation of Move\n");
            }

            //might cause problem bc of the derefferencing of movement struct 
            server_response.move = moveRequest->move;
            if(send(gameBoards[moveRequest->board_id].clientBlackId, serverResponse, sizeof(serverResponse),0)<0){
            perror("[requestToMove] Error Sending Confirmation of Move\n");
            }
        }
        else{
            if(send(gameBoards[moveRequest->board_id].clientBlackId, serverResponse, sizeof(serverResponse),0)<0){
            perror("[requestToMove] Error Sending Confirmation of Move\n");
            }

            //might cause problem bc of the derefferencing of movement struct 
            server_response.move = moveRequest->move;
            if(send(gameBoards[moveRequest->board_id].clientWhiteId, serverResponse, sizeof(serverResponse),0)<0){
            perror("[requestToMove] Error Sending Confirmation of Move\n");
            }
        }
        boardsInUse[moveRequest->board_id] = false;

    }
    else{
        server_response.sc_comm = moveOutput;
        responseToPayload(serverResponse, server_response);

        if(send(client_socket, serverResponse, sizeof(serverResponse),0)<0){
            perror("[requestToMove] Error Sending Confirmation of Move\n");
        }
    }
}

int handle_request(int c_socket){
    struct response client_request;
    char clientPayload[BUFFER_SIZE];

    client_request.client_id = 0;
    client_request.sc_comm = 0;
    client_request.board_id = 0;
    client_request.move.startRow = 0;
    client_request.move.startCol = 0;
    client_request.move.endRow = 0;
    client_request.move.endCol = 0;

    printf("[handle_request] Waiting to recieve request...");
    if(recv(c_socket, &clientPayload, sizeof(clientPayload),0) <= 0){
        printf("Connnection Lost\n");
        return -1;
    }

    printf("Package Recieved\n");
    
    payloadToResponse(clientPayload, &client_request);
 
    printf("[handle_request] Package Contents:");
    printf(" %ld", client_request.client_id);
    printf(" %d", client_request.sc_comm);
    printf(" %ld", client_request.board_id);
    printf(" %d", client_request.move.startRow);
    printf(" %d", client_request.move.startCol);
    printf(" %d", client_request.move.endRow);
    printf(" %d\n", client_request.move.endCol);

    
    printf("[handle_request] Client ID: %ld\n", client_request.client_id);
    printf("[handle_request] Board ID: %ld\n", client_request.board_id);
    printf("[handle_request] Comm: %s\n", MOVEERR_TO_STRING(client_request.sc_comm));
    printf("[handle_request] Start:");
    printf(" %d", client_request.move.startRow);
    printf(" %d\n", client_request.move.startCol);

    printf("[handle_request] MoveTo:");
    printf(" %d", client_request.move.endRow);
    printf(" %d\n", client_request.move.endCol);

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
    
    FD_SET(s_socket, &current_sockets);
    //printf("[handle_connection] Server Socket: %ld with count: %d\n", current_sockets.__fds_bits[0], sockets_connected - 1);

    int maxConnSocket = 1;
    printf("[handle_connection] Server File Descriptor: %d\n", s_socket);
    printf("[handle_connection] Maximum Number of Possible Clients: %d\n", FD_SETSIZE - 1);
    printf("[handle_connection] Server Ready to Connect to Clients\n");

    while(true){
        printf("\n[handle_connection] Number of Clients Connected to the Server: %d \n", sockets_connected - 1);
        
        printf("[handle_connection] Listening for Activity on File Descriptors: \n");
        printf("[handle_connection] ");
        drawSocketFD(&current_sockets, 10);
        
        ready_sockets = current_sockets;

        //printf("HC: Current Server Socket: %d with count: %d\n", ready_sockets.fd_array[0], ready_sockets.fd_count);

        //fd_setsize is max number allowed in set size, defined as 64 in our system
        //         maxConnSocket
        if (select(FD_SETSIZE, &ready_sockets, NULL, NULL, NULL ) < 0){
            perror("[handle_connection] Select Failed\n");
            exit(EXIT_FAILURE);
        }

        for (int i=0; i < FD_SETSIZE; i++){
            if(FD_ISSET(i, &ready_sockets)){
                if (i == s_socket){
                    printf("[handle_connection] Client attempt to connect...\n");
                    int client_socket = accept(s_socket, NULL, NULL);
                    FD_SET(client_socket, &current_sockets);
                    FD_CLR(s_socket, &ready_sockets);
                    maxConnSocket++;
                    printf("[handle_connection] Client %d Accepted to the Server\n", client_socket);
                    sockets_connected++;
                }
                else{
                    //printf("HC: Current Server Socket: %d \n", ready_sockets.fd_array[0]);
                    printf("[handle_connection] Handling Request for socket: %d\n", i);
                    if(handle_request(i)){
                        printf("[handle_connection] Client Lost Connection to Server\n");
                        FD_CLR(i, &current_sockets);
                        sockets_connected--;

                        //notify client that they have won through opp's forfit
                        //If they are waiting for thier opp's move
                        char notifyClientOfForfit[] = "0 14 0 0 0 0 0";
                        if(send(connectedClientInfo[i].clientOpponentsFD, notifyClientOfForfit, sizeof(notifyClientOfForfit), 0) < 0){
                            perror("[handle_connection] Error Notifing Opponent they won through forfit\n");
                        }
                        
                        // allows client to know when client has disconnected if 
                        // it is there turn and they havent sent a move yet
                        // used in requestToMove
                        connectedClientInfo[connectedClientInfo[i].clientOpponentsFD].isOpponentConnected = false;

                        connectedClientInfo[i].clientOpponentsFD = 0;
                        connectedClientInfo[i].clientBoardID = 0;
                        connectedClientInfo[i].clientBoardID = false;

                        //open up board space
                        boardsInUse[connectedClientInfo[i].clientBoardID] = false;

                    }
                    printf("[handle_connection] Handle Request Success\n");
                }
            }
        }
    }
    


}

int main(int argc, char *argv[]){
    //Prevents Server from Crashing when Last Client Disconnects
    signal(SIGPIPE, SIG_IGN);
    
    titleServerECO_draw();
    int sockfd, socket_port, accepted_client, opt;
    struct sockaddr_in addr;
    
    //setting time as seed for rand()
    srand(time(0));

	printf("\n[MAIN] Creating Socket\n");

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1){
        //add error check info
        printf("[MAIN] Could not create socket error num: %d\n", errno);
    }
    printf("[MAIN] Server Socket File Discriptor Created: %d\n", sockfd);

    int optval = 1;
	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (void *)&optval, sizeof(optval));

    //initialize address & port for the server
    struct sockaddr_in ECOserver_address;
    ECOserver_address.sin_family = AF_INET;

    printf("[MAIN] Setting Server Port as: %d\n", SERVER_PORT);
    ECOserver_address.sin_port = htons(SERVER_PORT);

    //ECOserver_address.sin_addr.s_addr = INADDR_ANY;

    //Debugging tool Enabled in header file
    if (USE_LOCAL_HOST){
        printf("[MAIN] Setting Server Address as: 127.0.0.1\n");
        ECOserver_address.sin_addr.s_addr = inet_addr("127.0.0.1");
    }
    else{
        printf("[MAIN] Setting Server Address as: %s\n", SERVER_ADDRESS);
        ECOserver_address.sin_addr.s_addr = inet_addr(SERVER_ADDRESS);
    }

    //bind address & port to our socket
    printf("[MAIN] Binding Address to Server Socket\n");
    if(bind(sockfd, (struct sockaddr *)&ECOserver_address,sizeof(ECOserver_address)) < 0){
		printf("%d\n", errno);
        perror("[Main] Unable to bind socket\n");
        
		return EXIT_FAILURE;
    }

    //listen for connections from client/s
    printf("[MAIN] Listening on Server Socket\n");
    if(listen(sockfd, 5) < 0){
		perror("[Main] Unable to listen on socket\n");
		return EXIT_FAILURE;
    }
    
    printf("[MAIN] Server is Ready to Accept Clients\n\n");
    handle_connection(sockfd);

    close(sockfd);

}
