#include <stdio.h>
#include <stdlib.h>
#include <winsock2.h>
#include <pthread.h>
#include <errno.h>

#include "ECO_chess.h"

#pragma comment(lib,"ws2_32.lib") //Winsock Library

struct movement{
    u_int64 startRow;
    u_int64 startCol;
    u_int64 endRow;
    u_int64 endCol;
};

struct request{
    u_int64 client_id;
    u_int64 board_id;
    enum moveErr sc_comm;
    struct movement move_req;

};

struct response{
    u_int64 client_id;
    u_int64 board_id;
    enum moveErr sc_comm;
    struct movement opp_move;
};

struct clientSocket{
    int c_socket;
    //char *name;
    boolean sucsessfulConnection;
};

struct s_chessBoard{
    struct chess_board * board;
    u_int64 clientWhiteId;
    u_int64 clinetBlackId;
    //int board_id;
};

void handle_connections(int);

struct clientSocket * acceptConnection(int);

struct s_chessBoard * initializeBoard(struct clientSocket *, struct clientSocket *);

void server_chess_run(struct s_chessBoard);

void client_chess_run(int);

void initializeMovement(struct movement * move, u_int64 startRow, u_int64 startCol, u_int64 endRow, u_int64 endCol);


