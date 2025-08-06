#include "ECO_common.h"

//MUST CHANGE THIS HERE AND ON ECO_server.c TO WORK PROPERLY
int CHANGE_TURNS_ENABLED = true;

enum player currentColor = EMPTY;

void initializeMovement(struct movement * move, char startRow, char startCol, char endRow, char endCol){
    move->startRow = startRow - '1';
    move->startCol = tolower(startCol) - 'a';
    move->endRow = endRow - '1';
    move->endCol = tolower(endCol) - 'a';
}

void sendMoveRequestToServer(int socket, struct chess_board* client_board, struct request * clientRequest, struct response * serverResponse, char*buffer){
    int loopCondition = 0;
    do{
        drawChessBoardInClient(client_board, currentColor);
        if(serverResponse->sc_comm == VALID_PLACEMENT){
            printf("MR: Opponent moved %c%c to %c%c. ", serverResponse->opp_move.startCol + 'a', serverResponse->opp_move.startRow + '1',
                                                serverResponse->opp_move.endCol + 'a', serverResponse->opp_move.endRow + '1');
        }
        else if(serverResponse->sc_comm != VALID_PLACEMENT || serverResponse->sc_comm != GAME_START_WHITE){
            printf("MR: %s Please Try Again and ", MOVEERR_TO_STRING(serverResponse->sc_comm));
        }
        else{
            printf("MR: ");
        }
        printf("Enter Movement (in form \"a2 a3\"):");
        fflush(stdin);
        fgets(buffer, BUFFER_SIZE, stdin);
        printf("\n");

        printf("Sending Move...\n");
        initializeMovement(&clientRequest->move_req, buffer[1], buffer[0], buffer[4], buffer[3]);
        clientRequest->sc_comm = MOVE_REQUEST;
        printf("Verifying request: client ID - %ld board ID - %ld request - %s move - %d%d %d%d\n",
                    clientRequest->client_id, clientRequest->board_id, MOVEERR_TO_STRING(clientRequest->sc_comm), 
                    clientRequest->move_req.startRow, clientRequest->move_req.startCol, clientRequest->move_req.endRow, clientRequest->move_req.endCol);

        if(send(socket, (char*)clientRequest, sizeof(struct request),0)<1){
            perror("Failed Sending Move");
            return;
        }
        printf("Movement Sent\n");
        
        printf("Waiting for Server Response...");
        if(recv(socket, (char*)serverResponse, sizeof(struct response),0)<1){
            perror("Failed to recieve response");
            return;
        }
        printf("recieved\n");
        if (serverResponse->sc_comm == VALID_PLACEMENT || serverResponse->sc_comm == WINNING_MOVE){
            chessClient_move(client_board, clientRequest->move_req.startRow,clientRequest->move_req.startCol,
                            clientRequest->move_req.endRow, clientRequest->move_req.endCol);
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
    struct request clientRequest;
    struct response serverResponse;
    struct chess_board * client_board = malloc(sizeof(struct chess_board));
    //client_board.spaces = malloc(sizeof(struct chess_space)*64);

    //recieve timeout
    //DWORD timeout = (1) * 1000;
    //setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof timeout);

    u_int64_t clientId, boardId;
    char startRow, startCol, endRow, endCol;
    char buffer[BUFFER_SIZE];

    //send request to start a game to server
    clientRequest.client_id = 0;
    clientRequest.sc_comm = START_GAME_REQUEST;
    clientRequest.board_id = 0;
    clientRequest.move_req.startRow = 0;
    clientRequest.move_req.startCol = 0;
    clientRequest.move_req.endRow = 0;
    clientRequest.move_req.endCol = 0;



    printf("CR: Sending Request to Server to Start Game...");
    if(send(socket, (char*)&clientRequest, sizeof(struct request),0)<1){
        perror("Failed Sending Request to Start Game");
        return;
    }
    printf("Sent\n");
    printf("CR: Client Request: %s, %d\n", MOVEERR_TO_STRING(clientRequest.sc_comm), clientRequest.sc_comm);

    //recv confimation from server
    printf("CR: Waiting for response from server...");
    if(recv(socket, (char*)&serverResponse, sizeof(struct response),0)<1){
        perror("Failed to recieve response");
        return;
    }
    printf("Recieved\n");

    clientRequest.client_id = serverResponse.client_id;
    clientRequest.board_id = serverResponse.board_id;

    
    switch(serverResponse.sc_comm){
        
        //if white, create board & respond to server with initial move
        case GAME_START_WHITE:
        chess_boardCreate(client_board);
        currentColor = WHITE;
        drawChessBoardInClient(client_board, currentColor);
        
        printf("CR: Game found! You are White, ");
        sendMoveRequestToServer(socket, client_board, &clientRequest, &serverResponse, buffer);

        break;

        //if black, create board & recv for opp. intial move
        case GAME_START_BLACK:
        chess_boardCreate(client_board);
        currentColor = BLACK;
        drawChessBoardInClient(client_board, currentColor);
        printf("CR: Game found! You are Black, ");
        break;
        
    }
    
    
    //for while that starts with a recv, then send move, 
    //recv confimation of said move, make move on board & draw, recv opp. move
    do{
        printf("CR: Waiting on Opponent's Move...");
        recv(socket, (char*)&serverResponse, sizeof(struct response),0);
        if(serverResponse.sc_comm == WINNING_MOVE){
            chessClient_move(client_board, serverResponse.opp_move.startRow, serverResponse.opp_move.startCol,
                            serverResponse.opp_move.endRow, serverResponse.opp_move.endCol);
            drawChessBoardInClient(client_board, currentColor);
            printf("Opponent Won! ");
            gameCondition = 0;
        }
        else{
            chessClient_move(client_board, serverResponse.opp_move.startRow, serverResponse.opp_move.startCol,
                            serverResponse.opp_move.endRow, serverResponse.opp_move.endCol);
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
    struct in_addr any_address;
    int socket_port = SERVER_PORT, client_socket;
    char buffer[512];
    int mainLoopCondition = 1;

    //set port from command line
    if (argc > 1) {
		socket_port = strtol(argv[1], NULL, 10);
		printf("\nINFO: setting server port as: %d\n", socket_port);
	} else {
		//fprintf(stderr, USAGE_STRING, argv[0]);
        socket_port = LOCAL_PORT;
	}
	
	printf("Initialised.\n");

    //create socket for client
    if((client_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1){
        perror("Could not create socket");
        return EXIT_FAILURE;
    }

    printf("M: Socket Created.\n");

    printf("M: Connecting to Server...");
    struct sockaddr_in ECOserver_address;
    ECOserver_address.sin_family = AF_INET;
    ECOserver_address.sin_port = htons(SERVER_PORT);
    ECOserver_address.sin_addr.s_addr = inet_addr(SERVER_ADDRESS);
    //ECOserver_address.sin_addr.s_addr = INADDR_ANY;
    if(connect(client_socket, (struct sockaddr *)&ECOserver_address, sizeof(ECOserver_address)) < 0){
        printf("%d\n", errno);
        perror("Failed to Connect to Server");
        printf("\nM: Failed to Connect to Server, Attempting to Reconnect...");
        int i;
        do{
           i = connect(client_socket, (struct sockaddr *)&ECOserver_address, sizeof(ECOserver_address));
        }while(i<0);
        
    }
    printf("Connected to Server Succesfully\n");
    
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
    close(client_socket);
}
