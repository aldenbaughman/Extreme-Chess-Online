//chess game includes
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <time.h>
#include <stdbool.h>
#include <string.h>

#define LOCAL_PORT (3333)

#define CHESS_BOARD_SIZE (8)
#define BUFFER_SIZE (256)

//Developer Options
#define SWITCH_TEAMS_AFTER_MOVEMENT (1)
#define CLEAR_SCREEN_AFTER_MOVE (0)

//server-client includes
#include <sys/select.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

//#include <pthread.h>
#include <errno.h>

//server-client constants
//#pragma comment(lib,"ws2_32.lib") //Winsock Library

//#define SERVER_ADDRESS "172.236.241.97"
#define SERVER_ADDRESS "127.0.0.1"
#define SERVER_PORT 5555

#define MATCHMAKING_QUEUE_SIZE (256)

//chess stuff
enum piece{
    PAWN = 0,
    ROOK,
    KNIGHT,
    BISHOP, 
    QUEEN,
    KING,
    NO_PIECE
};

enum player{
    WHITE = 0,
    BLACK,
    EMPTY
};

enum moveErr{
    //enums for chess game
    MOVEMENT_OUT_OF_BOUNDS = 0,
    START_MOVE_EMPTY,
    TARGETING_OWN_PIECE,
    MOVING_WRONG_PLAYER,
    INVALID_PLACEMENT_MOVE_ERROR,
    PIECE_IN_PATH,
    IN_CHECK_MOVE_ERROR,
    VALID_PLACEMENT,
    WINNING_MOVE,
    
    //enums for server-client communication
    GAME_START_WHITE,
    GAME_START_BLACK,
    START_GAME_REQUEST,
    MOVE_REQUEST,
    MOVE_RESPONSE

};

static const char * const piece_stringsText [] = {
    "Pawn",
    "Rook",
    "Knight",
    "Bishop", 
    "Queen",
    "King"
};

static const const char * const piece_stringsBoard [] = {
    "PN",
    "RK",
    "KN",
    "BS", 
    "QN",
    "KG",
    "  "
};

static const char * const player_stringText [] = {
    "White",
    "Black",
    "Empty"
};

static const char * const player_stringBoard [] = {
    "w",
    "b",
    " "
};

static const char * const moveErr_messages [] = {
    "Movement out of Board",
    "Space Chosen to Move is Empty",
    "Attacking own Piece",
    "Moving Other Player's Piece",
    "Invalid movement for chosen piece",
    "Piece in way try again!",
    "In check... move to protect",
    "Correct Placement!",
    " Captured their Opponents King and Won! Press Enter to Continue",
    "Game found you are White",
    "Game found you are Black",
    "Client Requesting to Start a Game",
    "Client Requesting piece movement",
    "Server Response to piece movement"

};                                  

static const char const column_letters[] = "             A       B       C       D       E       F       G       H\n";
static const char const rowLine_draw[] = "         -----------------------------------------------------------------\n";

#define PIECE_TO_STRING_TEXT(piece)		    \
    (piece_stringsText[piece])

#define PIECE_TO_STRING_BOARD(piece)		\
    (piece_stringsBoard[piece])

#define PLAYER_TO_STRING_TEXT(player)		\
    (player_stringText[player])

#define PLAYER_TO_STRING_BOARD(player)		\
    (player_stringBoard[player])

#define MOVEERR_TO_STRING(message)          \
    (moveErr_messages[message])

#define matrixSpaceAt(x,y)                  \
    (x*(CHESS_BOARD_SIZE)+y)


struct chess_space {
    int isEmpty;
    enum piece residingPiece;
    enum player pieceOwner; //white is 0, black is 1, 2 is empty
    int notMoved;
};

struct chess_board {
    struct chess_space spaces[CHESS_BOARD_SIZE * CHESS_BOARD_SIZE];
    enum piece whiteCaptures[16];
    enum piece blackCaptures[16];
    int numWhiteCaptures;
    int numBlackCaptures;
    enum player board_turn;
    enum player inCheck;

};

struct SMALL_RECT {
    short Left;
    short Top;
    short Right;
    short Bottom;
};

void chess_boardCreate(struct chess_board *);

void chess_boardDelete(struct chess_board *);

void drawChessBoardInClient(struct chess_board *, enum player);

void change_turn(struct chess_board *);

void titleCIC_draw();

void titleClientECO_draw();

void titleServerECO_draw();

void playAgain_draw();

enum moveErr chessServer_move(struct chess_board *,int startMove_row, int startMove_col,
                    int endMove_row, int endMove_col);

void chessClient_move(struct chess_board *,int startMove_row, int startMove_col,
                    int endMove_row, int endMove_col);

enum moveErr pawn_move(struct chess_board *,int startMove_row, int startMove_col,
                    int endMove_row, int endMove_col);

enum moveErr rook_move(struct chess_board *,int startMove_row, int startMove_col,
                    int endMove_row, int endMove_col);

enum moveErr knight_move(struct chess_board *,int startMove_row, int startMove_col,
                    int endMove_row, int endMove_col);

enum moveErr bishop_move(struct chess_board *,int startMove_row, int startMove_col,
                    int endMove_row, int endMove_col);

enum moveErr queen_move(struct chess_board *,int startMove_row, int startMove_col,
                    int endMove_row, int endMove_col);

enum moveErr king_move(struct chess_board *,int startMove_row, int startMove_col,
                    int endMove_row, int endMove_col);


void chess_game();


//server-client structs
struct movement{
    int startRow;
    int startCol;
    int endRow;
    int endCol;
};


struct response{
    u_int64_t client_id;
    u_int64_t board_id;
    enum moveErr sc_comm;
    struct movement move;
    char opponentUsername[32];
};

struct clientSocket{
    int c_socket;
    //char *name;
    bool sucsessfulConnection;
};

struct s_chessBoard{
    struct chess_board board;
    u_int64_t clientWhiteId;
    u_int64_t clientBlackId;
    int board_id;
};

struct queue {
	int* waiting_clients;
	int read;
	int write;
	int size;
	int maxLength;
};

void handle_connections(int);

int openBoardSpace();

void initializeBoard(int, u_int64_t, u_int64_t);

void initializeMovement(struct movement * move, char startRow, char startCol, char endRow, char endCol);

void sendMoveRequestToServer(int socket, struct chess_board* client_board, struct response * clientRequest, struct response * serverResponse, char*buffer);

void chess_run_client(int);

void responseToPayload(char* payload, struct response responseInfo);

int splitPayloadBySpaces(char* payload, struct response responseInfo);



