#include "ECO_common.h"

//MUST CHANGE THIS HERE AND ON ECO_server.c TO WORK PROPERLY
int CHANGE_TURNS_ENABLED = TRUE;

enum player currentColor = EMPTY;

void initializeMovement(struct movement * move, char startRow, char startCol, char endRow, char endCol){
    move->startRow = startRow - '1';
    move->startCol = tolower(startCol) - 'a';
    move->endRow = endRow - '1';
    move->endCol = tolower(endCol) - 'a';
}

void sendMoveRequestToServer(int socket, struct chess_board* client_board, struct response * clientRequest, struct response * serverResponse, char*buffer){
    int loopCondition = 0;
    char clientPayload[BUFFER_SIZE];
    char serverPayload[BUFFER_SIZE];

    do{
        drawChessBoardInClient(client_board, currentColor);
        if(serverResponse->sc_comm == VALID_PLACEMENT){
            printf("[sendMoveRequestToServer] Opponent moved %c%c to %c%c. ", serverResponse->move.startCol + 'a', 
                                                                              serverResponse->move.startRow + '1',
                                                                              serverResponse->move.endCol + 'a', 
                                                                              serverResponse->move.endRow + '1');
        }
        else if (serverResponse->sc_comm == GAME_START_WHITE){
            printf("[sendMoveRequestToServer] Game Found! You are White\n");
        }
        else if(serverResponse->sc_comm != VALID_PLACEMENT && serverResponse->sc_comm != GAME_START_WHITE){
            printf("[sendMoveRequestToServer] %s Please Try Again and ", MOVEERR_TO_STRING(serverResponse->sc_comm));
        }
        else{
            printf("[sendMoveRequestToServer] ");
        }
        printf("Enter Movement (in form \"a2 a3\"):");
        fflush(stdin);
        fgets(buffer, BUFFER_SIZE, stdin);
        printf("\n");

        printf("Sending Move...\n");
        initializeMovement(&clientRequest->move, buffer[1], buffer[0], buffer[4], buffer[3]);
        clientRequest->sc_comm = MOVE_REQUEST;
        printf("[sendMoveRequestToServer] Requesting Move: client ID - %ld board ID - %ld request - %s move - %d%d %d%d\n",
                                            clientRequest->client_id, 
                                            clientRequest->board_id, 
                                            MOVEERR_TO_STRING(clientRequest->sc_comm), 
                                            clientRequest->move.startRow,
                                            clientRequest->move.startCol,
                                            clientRequest->move.endRow,
                                            clientRequest->move.endCol);

        responseToPayload(clientPayload, *clientRequest);  
        
        if(send(socket, clientPayload, sizeof(clientPayload),0)<1){
            perror("[sendMoveRequestToServer] Failed Sending Move");
            return;
        }
        printf("[sendMoveRequestToServer] Movement Sent\n");
        
        printf("[sendMoveRequestToServer] Waiting for Server Response\n");
        if(recv(socket, serverPayload, sizeof(serverPayload),0)<1){
            printf("[sendMoveRequestToServer] Failed to recieve response %d\n" , WSAGetLastError());
            return;
        }
        
        payloadToResponse(serverPayload, serverResponse);

        printf("[sendMoveRequestToServer] Response from Server: %s\n", MOVEERR_TO_STRING(serverResponse->sc_comm));
        if (serverResponse->sc_comm == VALID_PLACEMENT || serverResponse->sc_comm == WINNING_MOVE){
            chessClient_move(client_board, clientRequest->move.startRow,clientRequest->move.startCol,
                            clientRequest->move.endRow, clientRequest->move.endCol);
            if(CHANGE_TURNS_ENABLED){
                change_turn(client_board);
            }
            drawChessBoardInClient(client_board, currentColor);
            loopCondition = 0;
        }
        else{
            loopCondition = 1;
        }
    }while(loopCondition);
}


void chess_run_client(int socket){
    int gameCondition = 1;
    struct response clientRequest;
    char requestPayload[BUFFER_SIZE];
    struct response serverResponse;
    char responsePayload[BUFFER_SIZE];
    struct chess_board * client_board = malloc(sizeof(struct chess_board));
    //client_board.spaces = malloc(sizeof(struct chess_space)*64);

    //recieve timeout
    //DWORD timeout = (1) * 1000;
    //setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof timeout);

    u_int64 clientId, boardId;
    char startRow, startCol, endRow, endCol;
    char buffer[BUFFER_SIZE];

    //send request to start a game to server
    clientRequest.client_id = 0;
    clientRequest.sc_comm = START_GAME_REQUEST;
    clientRequest.board_id = 0;
    clientRequest.move.startRow = 0;
    clientRequest.move.startCol = 0;
    clientRequest.move.endRow = 0;
    clientRequest.move.endCol = 0;

    responseToPayload(requestPayload, clientRequest);   

    printf("[chess_run_client] Sending Request to Server to Start Game...");
    if(send(socket, requestPayload, sizeof(requestPayload),0)<1){
        perror("Failed Sending Request to Start Game");
        return;
    }
    printf("Sent\n");

    //recv confimation from server
    printf("[chess_run_client] Waiting for response from server\n");
    if(recv(socket, responsePayload, sizeof(responsePayload),0)<1){
        perror("[chess_run_client] Failed to recieve response");
        return;
    }

    payloadToResponse(responsePayload, &serverResponse);

    clientRequest.client_id = serverResponse.client_id;
    clientRequest.board_id = serverResponse.board_id;

    printf("[chess_run_client] Server Response:   sc_comm = %s   Client ID = %ld   Board ID = %ld\n", MOVEERR_TO_STRING(serverResponse.sc_comm), serverResponse.client_id, serverResponse.board_id);

    switch(serverResponse.sc_comm){
        
        //if white, create board & respond to server with initial move
        case GAME_START_WHITE:
        chess_boardCreate(client_board);
        currentColor = WHITE;
        //drawChessBoardInClient(client_board, currentColor);
        
        sendMoveRequestToServer(socket, client_board, &clientRequest, &serverResponse, buffer);

        break;

        //if black, create board & recv for opp. intial move
        case GAME_START_BLACK:
        chess_boardCreate(client_board);
        currentColor = BLACK;

        drawChessBoardInClient(client_board, currentColor);
        printf("[chess_run_client] Game found! You are Black, waiting on Opponent's Move...\n");

        break;
        
    }
    
    
    //for while that starts with a recv, then send move, 
    //recv confimation of said move, make move on board & draw, recv opp. move
    do{
        recv(socket, responsePayload, sizeof(responsePayload),0);
        payloadToResponse(responsePayload, &serverResponse);

        if(serverResponse.sc_comm == WINNING_MOVE){
            chessClient_move(client_board, serverResponse.move.startRow, serverResponse.move.startCol,
                            serverResponse.move.endRow, serverResponse.move.endCol);
            drawChessBoardInClient(client_board, currentColor);
            printf("Opponent Won! ");
            gameCondition = 0;
        }
        else{
            chessClient_move(client_board, serverResponse.move.startRow, serverResponse.move.startCol,
                            serverResponse.move.endRow, serverResponse.move.endCol);
            drawChessBoardInClient(client_board, currentColor);
            sendMoveRequestToServer(socket, client_board, &clientRequest, &serverResponse, buffer);
            drawChessBoardInClient(client_board, currentColor);
            if(serverResponse.sc_comm == WINNING_MOVE){
                printf("You Won! ");
                gameCondition = 0;
            }
        }
    }while(gameCondition);
    
    printf("Game Over.\n");
}

int main(int argc, char *argv[]){    
    titleClientECO_draw();
    WSADATA wsa;
    struct in_addr any_address;
    int socket_port, client_socket;
    char buffer[512];
    int mainLoopCondition = 1;

    printf("M: Initialising Winsock...");
	//Initializes WinSock library
    if (WSAStartup(MAKEWORD(2,2),&wsa) != 0)
	{
		printf("Failed. Error Code : %d",WSAGetLastError());
		return 1;
	}
	
	printf("Initialised.\n");

    printf("[MAIN] Creating Client Socket\n");

    //create socket for client
    if((client_socket = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET){
        perror("[MAIN] Could not create socket");
        return EXIT_FAILURE;
    }

    printf("[MAIN] Socket Created: %d\n", client_socket);

    printf("[MAIN] Connecting to Server\n");
    struct sockaddr_in ECOserver_address;
    ECOserver_address.sin_family = AF_INET;
    ECOserver_address.sin_port = htons(SERVER_PORT);
    ECOserver_address.sin_addr.s_addr = inet_addr(SERVER_ADDRESS);
    if(connect(client_socket, (struct sockaddr *)&ECOserver_address, sizeof(ECOserver_address)) < 0){
        //perror("Failed to Connect to Server");
        printf("\nM: Failed to Connect to Server, Attempting to Reconnect...");
        int i;
        do{
           i = connect(client_socket, (struct sockaddr *)&ECOserver_address, sizeof(ECOserver_address));
        }while(i<0);
        
    }
    printf("[MAIN] Connected to Server Succesfully\n");
    
    do{
        chess_run_client(client_socket);

        playAgain_draw();
        printf("\n                 Press Enter to Requeue for Another Game or q to Quit: ");
        fflush(stdin);
        fgets(buffer, 256, stdin);
        if(buffer[0] == 'q'){
            mainLoopCondition = 0;
        }
    }while(mainLoopCondition);
    closesocket(client_socket);
    WSACleanup();   
}
