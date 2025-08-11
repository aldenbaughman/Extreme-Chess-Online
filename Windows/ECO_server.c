
#include "ECO_common.h"

#define NUM_MAX_CHESS_BOARDS (256)

//MUST CHANGE THIS HERE AND ON ECO_client.c TO WORK PROPERLY
int CHANGE_TURNS_ENABLED  = TRUE;

struct s_chessBoard gameBoards[NUM_MAX_CHESS_BOARDS];
int boardsInUse[NUM_MAX_CHESS_BOARDS] = {0};

//struct queue* MM_queue = NULL;

int matched_client1= -1;
int matched_client2= -1;

int openBoardSpace(){
    int i = 0;
    //if a board is being used, check next board
    while(boardsInUse[i]){
        i++;
    }
    return i;
}

void drawSocketFD(fd_set sockets){
    printf("[");
    for(int i = 0; i < sockets.fd_count;i++){
        printf(" %d ",sockets.fd_array[i]);
    }
    printf("]\n");
}

void initializeBoard(int s_boardId,u_int64 client1, u_int64 client2){
    printf("IB: creating chess board\n");
    gameBoards[s_boardId].board.spaces[matrixSpaceAt(0,0)].residingPiece = PAWN;
    //printf("Piece at board 0, space 0,0: %s\n", PIECE_TO_STRING_TEXT(gameBoards[s_boardId].board.spaces[matrixSpaceAt(0,0)].residingPiece));
    //server_board->board = malloc(sizeof(struct chess_board));
    chess_boardCreate(&gameBoards[s_boardId].board);
    gameBoards[s_boardId].clientWhiteId = client1;
    gameBoards[s_boardId].clientBlackId = client2;
    boardsInUse[s_boardId] = TRUE;
}

void requestToStartGame(int c_socket){
    struct response server_response;
    char serverPayload[BUFFER_SIZE];

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
        server_response.move.startRow = 0;
        server_response.move.startCol = 0;
        server_response.move.endRow = 0;
        server_response.move.endCol = 0;

        responseToPayload(serverPayload, server_response);

        if(send(player_white, (char*)&server_response, sizeof(struct response),0)<0){
            perror("SG: Failed to Send start Game for White\n");
            exit(EXIT_FAILURE);
        }
        
        server_response.client_id = player_black;
        server_response.sc_comm = GAME_START_BLACK;

        responseToPayload(serverPayload, server_response);

        if(send(player_black, serverPayload, sizeof(serverPayload),0)<0){
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
    char serverResponse[BUFFER_SIZE];
    
    server_response.client_id = 0;
    server_response.sc_comm = 0;
    server_response.board_id = 0;
    server_response.move.startRow = 0;
    server_response.move.startCol = 0;
    server_response.move.endRow = 0;
    server_response.move.endCol = 0;

    enum moveErr moveOutput = chessServer_move(&gameBoards[moveRequest->board_id].board, moveRequest->move_req.startRow,
                        moveRequest->move_req.startCol, moveRequest->move_req.endRow, moveRequest->move_req.endCol);
        if (moveOutput == VALID_PLACEMENT){
            
            //response to player moving piece
            server_response.sc_comm = VALID_PLACEMENT;
            memcpy(&server_response.move, &moveRequest->move, sizeof(struct movement));

            responseToPayload(serverResponse, server_response);

            if(send(client_socket, serverResponse, sizeof(serverResponse),0)<0){
                    perror("Error Sending Confirmation of Move\n");
            }
            if(CHANGE_TURNS_ENABLED ){
                change_turn(&gameBoards[moveRequest->board_id].board);
            }
            //response to other player notifying them a move has been made and its thier turn
            memcpy(&server_response.opp_move, &moveRequest->move_req, sizeof(struct movement));
            if (client_socket == gameBoards[moveRequest->board_id].clientWhiteId){
                 if(send(gameBoards[moveRequest->board_id].clientBlackId, serverResponse, sizeof(serverResponse),0)<0){
                    perror("Error Sending Move to Opponent\n");
                }               
            }
            else{
                if(send(gameBoards[moveRequest->board_id].clientWhiteId, serverResponse, sizeof(serverResponse),0)<0){
                    perror("Error Sending Move to Opponent\n");
                }
            }
        }
        
        else if (moveOutput == WINNING_MOVE){
            server_response.sc_comm = WINNING_MOVE;
            responseToPayload(serverResponse, server_response);

            if(gameBoards[moveRequest->board_id].board.board_turn == WHITE){
                if(send(gameBoards[moveRequest->board_id].clientWhiteId, serverResponse, sizeof(serverResponse),0)<0){
                perror("Error Sending Confirmation of Move\n");
                }

                //might cause problem bc of the derefferencing of movement struct 
                server_response.opp_move = moveRequest->move_req;
                if(send(gameBoards[moveRequest->board_id].clientBlackId, serverResponse, sizeof(serverResponse),0)<0){
                perror("Error Sending Confirmation of Move\n");
                }
            }
            else{
                if(send(gameBoards[moveRequest->board_id].clientBlackId, serverResponse, sizeof(serverResponse),0)<0){
                perror("Error Sending Confirmation of Move\n");
                }

                //might cause problem bc of the derefferencing of movement struct 
                server_response.opp_move = moveRequest->move_req;
                if(send(gameBoards[moveRequest->board_id].clientWhiteId, serverResponse, sizeof(serverResponse),0)<0){
                perror("Error Sending Confirmation of Move\n");
                }
            }
            boardsInUse[moveRequest->board_id] = FALSE;

        }
        else{
            server_response.sc_comm = moveOutput;
            responseToPayload(serverResponse, server_response);
            
            if(send(client_socket, serverResponse, sizeof(serverResponse),0)<0){
                perror("Error Sending Confirmation of Move\n");
            }
        }
}

int handle_request(int c_socket){
    struct request client_request;
    char clientPayload[BUFFER_SIZE];

    client_request.client_id = 0;
    client_request.sc_comm = 0;
    client_request.board_id = 0;
    client_request.move.startRow = 0;
    client_request.move.startCol = 0;
    client_request.move.endRow = 0;
    client_request.move.endCol = 0;

    printf("HR: Waiting to recieve request...");
    if(recv(c_socket, &clientPayload, sizeof(clientPayload), 0) < ){
        printf("Connnection Lost\n");
        return 1;
    }

    printf("\nHR: clientPayload: %s\n", clientPayload);
    payloadToResponse(clientPayload, &client_request);

    printf("\nHR: Checking Integrity of response: ");
    printf(" %ld", client_request.client_id);
    printf(" %d", client_request.sc_comm);
    printf(" %ld", client_request.board_id);
    printf(" %d", client_request.move.startRow);
    printf(" %d", client_request.move.startCol);
    printf(" %d", client_request.move.endRow);
    printf(" %d\n", client_request.move.endCol);

    printf("package recieved\n");
    printf("HR: Verifying packet: client id %d ", client_request.client_id);
    printf("board id: %d", client_request.board_id);
    printf("\n");0
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

    //current_sockets is bitfield, this sets it all to zero
    FD_ZERO(&current_sockets);

    //add the main(server) socket to current sockets 
    FD_SET(s_socket,&current_sockets);
    //printf("HC: Server Socket: %d with count: %d\n", current_sockets.fd_array[0], current_sockets.fd_count - 1);

    int maxConnSocket = 1;

    printf("HC: Server Socket Set as: %d\n", s_socket);

    printf("HC: Server Ready to Connect to Clients\n");
    while(TRUE){
        printf("\nHC: Listening for Activity Sockets: ");
        drawSocketFD(current_sockets);
        printf("HC: Number of Clients Currently Connected to the Server: %d\n", current_sockets.fd_count - 1);
        ready_sockets = current_sockets;

        //printf("HC: Current Server Socket: %d with count: %d\n", ready_sockets.fd_array[0], ready_sockets.fd_count);

        //fd_setsize is max number allowed in set size, defined as 64 in our system
        if (select(maxConnSocket, &ready_sockets, NULL, NULL, NULL ) < 0){
            perror("HC: Select Failed\n");
            exit(EXIT_FAILURE);
        }
        printf("HC: Activity on Socket: ");
        drawSocketFD(ready_sockets);

        if (ready_sockets.fd_array[0] == s_socket){
            printf("HC: Client attempt to connect...");
            int client_socket = accept(s_socket, NULL, NULL);
            FD_SET(client_socket, &current_sockets);
            maxConnSocket++;
            printf("Client %d Accepted to the Server\n", client_socket);
        }
        else{
            //printf("HC: Current Server Socket: %d \n", ready_sockets.fd_array[0]);
            printf("HC: Handling Request...\n");
            if(handle_request(ready_sockets.fd_array[0])){
                printf("HC: Client Lost Connection to Server");
                FD_CLR(ready_sockets.fd_array[0], &current_sockets);
            }
            printf("HC: Handle Request Success\n");
        }
        /*
        for (int i = 0; i < maxConnSocket; i++){
            printf("FOR LOOP ITR # %d\n", i);
            printf("is socket %d set?: %d\n", ready_sockets.fd_array[i],FD_ISSET(ready_sockets.fd_array[i], &ready_sockets));
            if(FD_ISSET(ready_sockets.fd_array[i], &ready_sockets)){
                if (ready_sockets.fd_array[i] == s_socket){
                    printf("Client attempt to connect...");
                    int client_socket = accept(s_socket, NULL, NULL);
                    FD_SET(client_socket, &current_sockets);
                    drawSocketFD(current_sockets);
                    maxConnSocket++;
                    printf("Client %d Accepted to the Server\n", client_socket);
                }
                else{
                    printf("Current Server Socket: %d with count: %d\n", ready_sockets.fd_array[0], i);
                    printf("Handling Request...\n");
                    handle_request(ready_sockets.fd_array[i]);
                    printf("Success\n");
                    FD_CLR(ready_sockets.fd_array[i], &current_sockets);
                }
            }
            printf("restarting loop\n");
        }
        */
        //printf("HC: restarting loop\n");
    }
    


}

int main(int argc, char *argv[]){
    titleServerECO_draw();
    WSADATA wsa;
    int server_socket, socket_port, accepted_client, opt;
    //setting time as seed for rand()
    srand(time(0));
    
    while((opt = getopt(argc,argv, "c")) != -1){
		switch (opt){
			case 'c' :
				CHANGE_TURNS_ENABLED  = TRUE;
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
        socket_port = LOCAL_PORT;
	}

	
	printf("\nInitialising Winsock...");
	//Initializes WinSock library
    if (WSAStartup(MAKEWORD(2,2),&wsa) != 0)
	{
		printf("Failed. Error Code : %d",WSAGetLastError());
		return 1;
	}
	
	printf("Initialised.\n");

    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET){
        printf("Could not create socket : %d", WSAGetLastError());
    }
    printf("Socket Created.\n");

    int optval = 1;
	setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, (void *)&optval, sizeof(optval));

    //initialize address & port for the server
    struct sockaddr_in ECOserver_address;
    ECOserver_address.sin_family = AF_INET;
    ECOserver_address.sin_port = htons(socket_port);
    ECOserver_address.sin_addr.s_addr = inet_addr("127.0.0.1");

    //bind address & port to our socket
    if(bind(server_socket, (struct sockaddr *)&ECOserver_address,sizeof(ECOserver_address)) < 0){
		perror("Unable to bind socket");
		return EXIT_FAILURE;
    }

    //listen for connections from client/s
    if(listen(server_socket, 5) < 0){
		perror("Unable to listen on socket");
		return EXIT_FAILURE;
    }
    
    handle_connection(server_socket);

    closesocket(server_socket);
    WSACleanup();

}
