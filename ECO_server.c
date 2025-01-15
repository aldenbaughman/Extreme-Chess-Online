
#include "ECO_common.h"

struct s_chessBoard **gameBoards = NULL;

void handle_connection(int s_socket){
    while(1){
        printf("HC: Waiting for Clients to Connect...");
        struct clientSocket * client1 = acceptConnection(s_socket);
        printf("HC: Client1 connected,");
        struct clientSocket * client2 = acceptConnection(s_socket);
        printf("HC: Clinet2 connected\n");

        printf("HC: Initializing game board...");
        struct s_chessBoard * gameBoard = initializeBoard(client1, client2);
        printf("initialization successful\n");
        
        //if any of these fail, just add initializeBoard to this function for sake of scope
        printf("Testing Initialization of Game Board:\n");
        printf("Client 1 Socket: %d\n", gameBoard->client1.c_socket);
        printf("Client 2 Socket: %d\n", gameBoard->client2.c_socket);
        printf("space a8 on game board: %s", PIECE_TO_STRING_TEXT(gameBoard->board->spaces[matrixSpaceAt(0,7)].residingPiece));


        printf("HC: Creating thread...");
        pthread_t id;
        //pthread_create(&id, NULL, chess_run, gameBoard);
        printf("HC: Thread Created\n");

    }
}

struct clientSocket * acceptConnection(int server_socket){
    char client_name[32];
    int c_Socket = accept(server_socket, NULL, NULL);
    struct clientSocket * client_socket = malloc(sizeof(struct clientSocket));
    client_socket->c_socket = c_Socket;
    //client_socket->name = recv(c_Socket, client_name, sizeof(client_name),0);

    return client_socket;
}

struct s_chessBoard * initializeBoard(struct clientSocket * client1, struct clientSocket * client2){
    struct s_chessBoard * server_board = malloc(sizeof(struct s_chessBoard));
    chess_boardCreate(server_board->board);
    server_board->client1 = *client1;
    server_board->client2 = *client2;

    return server_board;

}

int main(int argc, char *argv[]){
    WSADATA wsa;
    int server_socket, socket_port, accepted_client;
    char test_message[256] = "Hello from da server :>";

    //set port from command line
    if (argc > 1) {
		socket_port = strtol(argv[1], NULL, 10);
		printf("INFO: setting server port as: %d\n", socket_port);
	} else {
		//fprintf(stderr, USAGE_STRING, argv[0]);
        perror("Server port not set");
		return EXIT_FAILURE;
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

    printf("INFO: Waiting for incomming connection...");
    
    handle_connections(server_socket);

    closesocket(server_socket);
    WSACleanup();

}
