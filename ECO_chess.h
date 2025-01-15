
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <windows.h>

#define CHESS_BOARD_SIZE (8)

//Developer Options
#define SWITCH_TEAMS_AFTER_MOVEMENT (1)
#define CLEAR_SCREEN_AFTER_MOVE (1)

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
    MOVEMENT_OUT_OF_BOUNDS = 0,
    START_MOVE_EMPTY,
    TARGETING_OWN_PIECE,
    MOVING_WRONG_PLAYER,
    INVALID_PLACEMENT_MOVE_ERROR,
    PIECE_IN_PATH,
    IN_CHECK_MOVE_ERROR,
    VALID_PLACEMENT,
    WINNING_MOVE,
    GAME_START_WHITE,
    GAME_START_BLACK,
    START_GAME_REQUEST

};

const char * piece_stringsText [] = {
    "Pawn",
    "Rook",
    "Knight",
    "Bishop", 
    "Queen",
    "King"
};

const char * piece_stringsBoard [] = {
    "PN",
    "RK",
    "KN",
    "BS", 
    "QN",
    "KG",
    "  "
};

const char * player_stringText [] = {
    "White",
    "Black",
    "Empty"
};

const char * player_stringBoard [] = {
    "w",
    "b",
    " "
};

const char * moveErr_messages [] = {
    "Movement out of Board try Again!",
    "Empty Space choosen to move try again!",
    "Attacking own piece try again!",
    "Moving Other Player's Piece",
    "Invalid movement for chosen piece try again!",
    "Piece in way try again!",
    "In check move to protect king!",
    "Correct Placement!",
    " Captured their Opponents King and Won! Press Enter to Continue"
    "Game found you are White",
    "Game found you are Black"

};                                  

const char column_letters[] = "             A       B       C       D       E       F       G       H\n";
const char rowLine_draw[] = "         -----------------------------------------------------------------\n";

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
    struct chess_space * spaces;
    enum piece * whiteCaptures;
    enum piece * blackCaptures;
    int numWhiteCaptures;
    int numBlackCaptures;
    enum player board_turn;
    enum player inCheck;

};

struct SMALL_RECT {
    SHORT Left;
    SHORT Top;
    SHORT Right;
    SHORT Bottom;
};

void chess_boardCreate(struct chess_board *);

void chess_boardDelete(struct chess_board *);

void chess_draw(struct chess_board *);

void txtFile_draw();

enum moveErr chess_move(struct chess_board *,int startMove_row, int startMove_col,
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
