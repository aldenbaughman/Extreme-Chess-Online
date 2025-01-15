#include "ECO_common.h"

initializeMovement(struct movement * move, char startRow, char startCol, char endRow, char endCol){
    move->startRow = startRow - '1';
    move->startCol = tolower(startCol) - 'a';
    move->endRow = endRow - '1';
    move->endCol = tolower(endCol) - 'a';
}

void sendMoveRequestToServer(int socket, struct chess_board* client_board, struct request * clientRequest, struct response * serverResponse, char*buffer){
    int loopCondition = 0;
    do{
        printf("Oppenent moved %c%d to %c%d. ", serverResponse->opp_move.startCol + 'a', serverResponse->opp_move.startRow + '1',
                                            serverResponse->opp_move.endCol + 'a', serverResponse->opp_move.endRow + '1');
        printf("Enter Movement (in form \"a2 a3\"):");
        fflush(stdin);
        fgets(buffer, BUFFER_SIZE, stdin);
        printf("\n");

        printf("Sending Move...");
        initializeMovement(&(clientRequest->move_req), buffer[1], buffer[0], buffer[4], buffer[3]);
        if(send(socket, &clientRequest, sizeof(struct request),0)<1){
            printf("Failed Sending Move %d\n" , WSAGetLastError());
            return;
        }
        printf("sent\n");
        
        printf("Waiting for Server Response...");
        if(recv(socket, &serverResponse, sizeof(struct response),0)<1){
            printf("Failed to recieve response %d\n" , WSAGetLastError());
            return;
        }
        printf("recieved");
        if (serverResponse->sc_comm == VALID_PLACEMENT){
            chess_move(client_board, clientRequest->move_req.startRow,clientRequest->move_req.startCol,
                            clientRequest->move_req.endRow, clientRequest->move_req.endCol);
            loopCondition = 0;
        }
        else{
            printf("\n%s ", MOVEERR_TO_STRING(serverResponse->sc_comm));
            loopCondition = 1;
        }
    }while(loopCondition);
}


void chess_run(int socket){
    int gameCondition = 1;
    struct request clientRequest;
    struct response serverResponse;
    struct chess_board * client_board = malloc(sizeof(struct chess_board));
    client_board->spaces = malloc(sizeof(struct chess_space)*64);

    u_int64 clientId, boardId;
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

    printf("Sending Requst to Server to Start Game...");
    while(send(socket, &clientRequest, sizeof(struct request),0)<1){
        printf("Failed Sending Request to Start Game %d\n" , WSAGetLastError());
        return;
    }
    printf("Sent\n");

    //recv confimation from server
    printf("Waiting for response from server...");
    if(recv(socket, &serverResponse, sizeof(struct response),0)<1){
        printf("Failed to recieve response %d\n" , WSAGetLastError());
        return;
    }
    printf("Recieved\n");

    clientRequest.client_id = serverResponse.client_id;
    clientRequest.board_id = serverResponse.board_id;

    
    switch(serverResponse.sc_comm){
        
        //if white, create board & respond to server with initial move
        case GAME_START_WHITE:
        chess_boardCreate(client_board);
        chess_draw(client_board);
        
        printf("Game found! You are White, ");
        sendMoveRequestToServer(socket, client_board, clientRequest, serverResponse, buffer);

        break;

        //if black, create board & recv for opp. intial move
        case GAME_START_BLACK:
        chess_boardCreate(client_board);
        chess_draw(client_board);
        printf("Game found! You are Black, ");
        break;
        
    }
    
    
    //for while that starts with a recv, then send move, 
    //recv confimation of said move, make move on board & draw, recv opp. move
    do{
        printf("Waiting on Opponent's Move...");
        recv(socket, &serverResponse, sizeof(struct response),0);
        chess_move(client_board, serverResponse.opp_move.startRow, serverResponse.opp_move.startCol,
                        serverResponse.opp_move.endRow, serverResponse.opp_move.endCol);
        chess_draw(client_board);
        sendMoveRequestToServer(socket, client_board, &clientRequest, &serverResponse, buffer);
        chess_draw(client_board);

    }while(gameCondition);
}

int main(int argc, char *argv[]){
    WSADATA wsa;
    struct in_addr any_address;
    int socket_port, client_socket;
    char recieved_message[512];
    int mainLoopCondition = 1;

    //set port from command line
    if (argc > 1) {
		socket_port = strtol(argv[1], NULL, 10);
		printf("\nINFO: setting server port as: %d\n", socket_port);
	} else {
		//fprintf(stderr, USAGE_STRING, argv[0]);
        perror("Server port not set");
		return EXIT_FAILURE;
	}

    printf("Initialising Winsock...");
	//Initializes WinSock library
    if (WSAStartup(MAKEWORD(2,2),&wsa) != 0)
	{
		printf("Failed. Error Code : %d",WSAGetLastError());
		return 1;
	}
	
	printf("Initialised.\n");

    //create socket for client
    if((client_socket = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET){
        perror("Could not create socket");
        return EXIT_FAILURE;
    }

    printf("Socket Created.\n");

    struct sockaddr_in ECOserver_address;
    ECOserver_address.sin_family = AF_INET;
    ECOserver_address.sin_port = htons(socket_port);
    ECOserver_address.sin_addr.s_addr = inet_addr("127.0.0.1");

    if(connect(client_socket, (struct sockaddr *)&ECOserver_address, sizeof(ECOserver_address)) < 0){
    //perror("Failed to Connect to Server");
    printf("Failed to Connect to Server: %d" , WSAGetLastError());
    return EXIT_FAILURE;
    }
    
    chess_run(client_socket);

    closesocket(client_socket);
    WSACleanup();   




    //connect to server
    

    printf("Connected to Server.\n");

    if(recv(client_socket, recieved_message, sizeof(recieved_message), 0)<1){
        printf("Failed to Receive pakage %d\n" , WSAGetLastError());
    }
    
    char confirmation[256] = "From Client: I have recieved your message! :3";
    if(send(client_socket, confirmation, sizeof(confirmation),0)<1){
        printf("Failed to Send pakage %d\n" , WSAGetLastError());
    }

    printf("Message From Server: %s", recieved_message);



}