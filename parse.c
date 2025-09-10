#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BOARD_DATA_EMPTY 0
#define BOARD_DATA_WHITE_KING 1
#define BOARD_DATA_WHITE_QUEEN 2
#define BOARD_DATA_WHITE_ROOK 3
#define BOARD_DATA_WHITE_BISHOP 4
#define BOARD_DATA_WHITE_KNIGHT 5
#define BOARD_DATA_WHITE_PAWN 6
#define BOARD_DATA_WHITE_PAWN_EP_CAPTURABLE 7
#define BOARD_DATA_BLACK_KING 9
#define BOARD_DATA_BLACK_QUEEN 10
#define BOARD_DATA_BLACK_ROOK 11
#define BOARD_DATA_BLACK_BISHOP 12
#define BOARD_DATA_BLACK_KNIGHT 13
#define BOARD_DATA_BLACK_PAWN 14
#define BOARD_DATA_BLACK_PAWN_EP_CAPTURABLE 15

#define CASTLING_KINGSIDE_WHITE 1
#define CASTLING_QUEENSIDE_WHITE 2
#define CASTLING_KINGSIDE_BLACK 4
#define CASTLING_QUEENSIDE_BLACK 8

#define PROMOTION_PIECE_QUEEN 0
#define PROMOTION_PIECE_ROOK 1
#define PROMOTION_PIECE_KNIGHT 2
#define PROMOTION_PIECE_BISHOP 3

#define PLAYER_MOVE_WHITE 0
#define PLAYER_MOVE_BLACK 1

#define FILE_A 0
#define FILE_B 1
#define FILE_C 2
#define FILE_D 3
#define FILE_E 4
#define FILE_F 5
#define FILE_G 6
#define FILE_H 7
#define RANK_1 0
#define RANK_2 1
#define RANK_3 2
#define RANK_4 3
#define RANK_5 4
#define RANK_6 5
#define RANK_7 6
#define RANK_8 7

#define PARSE_SUCCESS 0
#define PARSE_ERROR_SYNTAX_ERROR -1
#define PARSE_ERROR_INVALID_BOARD_STATE -2
#define PARSE_ERROR_INCOMPLETE_FEN -3

typedef unsigned int Rank;
typedef unsigned int File;
typedef unsigned int Piece;
typedef unsigned int PromotionPiece;
typedef unsigned int PlayerMove;
typedef unsigned int CastlingRights;

typedef struct {
    uint8_t Data[32];
    unsigned int HalfmoveClock : 7;
    unsigned int FullmoveCounter : 12;
    CastlingRights CastlingRights : 4;
    PlayerMove PlayerMove : 1;
    unsigned int Padding : 8;
} Board;

typedef struct {
    uint32_t PuzzleId; // 4 bytes
    uint16_t Rating; // 2 bytes
    uint16_t RatingDeviation; // 2 bytes
    uint16_t Moves[40]; // 80 bytes
    uint8_t NumMoves; // 1 byte
    Board Board; // 32 bytes
} Puzzle;

int ParseMoveString(const char* moveString, size_t moveLength, uint16_t* Moves, size_t maxMoves)
{
    assert(moveString);
    assert(Moves);

    const char* pos = moveString;
    size_t idx = 0;

    // Until string is not over and there is space in the array
    while (pos < moveString + moveLength && *pos && idx < maxMoves) {
        // Remove whitespace
        while (pos < moveString + moveLength && isspace(pos))
            pos++;

        uint16_t move = 0;

        if (pos + 3 < moveString + moveLength) {
            const File sourceFile = pos[0] - 'a';
            if (0 <= sourceFile && sourceFile < 8) {
                move |= sourceFile << 13;
            } else {
                return PARSE_ERROR_SYNTAX_ERROR;
            }

            const Rank sourceRank = pos[1] - '1';
            if (0 <= sourceRank && sourceRank < 8) {
                move |= sourceRank << 10;
            } else {
                return PARSE_ERROR_SYNTAX_ERROR;
            }

            const File destFile = pos[2] - 'a';
            if (0 <= destFile && destFile < 8) {
                move |= destFile << 7;
            } else {
                return PARSE_ERROR_SYNTAX_ERROR;
            }

            const Rank destRank = pos[3] - '1';
            if (0 <= destRank && destRank < 8) {
                move |= sourceFile << 4;
            } else {
                return PARSE_ERROR_SYNTAX_ERROR;
            }
        }

        if (pos + 4 < moveString + moveLength && !isspace(pos[4])) {
            switch (pos[4]) {
            case 'Q':
            case 'q':
                move |= PROMOTION_PIECE_QUEEN << 2;
                break;
            case 'R':
            case 'r':
                move |= PROMOTION_PIECE_ROOK << 2;
                break;
            case 'N':
            case 'n':
                move |= PROMOTION_PIECE_KNIGHT << 2;
                break;
            case 'B':
            case 'b':
                move |= PROMOTION_PIECE_BISHOP << 2;
                break;
            default:
                return PARSE_ERROR_SYNTAX_ERROR;
            }
        }

        Moves[idx++] = move;
    }

    return 0;
}

Piece BoardGetSquare(const Board* board, File file, Rank rank)
{
    int square_index = rank * 8 + file;
    int byte_index = square_index / 2;
    int is_upper_nibble = square_index % 2;

    if (is_upper_nibble) {
        return (board->Data[byte_index] >> 4) & 0x0F;
    } else {
        return board->Data[byte_index] & 0x0F;
    }
}

void BoardSetSquare(Board* board, File file, File rank, Piece piece)
{
    int square_index = rank * 8 + file;
    int byte_index = square_index / 2;
    int is_upper_nibble = square_index % 2;

    if (is_upper_nibble) {
        board->Data[byte_index] = (board->Data[byte_index] & 0x0F) | (piece << 4);
    } else {
        board->Data[byte_index] = (board->Data[byte_index] & 0xF0) | piece;
    }
}

int ParseFen(const char* fenString, size_t fenLength, Board* board)
{
    assert(fenString);
    assert(board);

    const char* pos = fenString;
    bool allFieldsParsed = false;

    // Skip initial spaces.
    while (pos < fenString + fenLength && isspace(*pos))
        ++pos;

    // Parse board state.
    Rank rank = RANK_8;
    File file = FILE_A;
    while (pos < fenString + fenLength && !isspace(*pos)) {
        if (file > 8)
            return PARSE_ERROR_INVALID_BOARD_STATE;
        switch (*pos) {
        case 'K':
            BoardSetSquare(board, file++, rank, BOARD_DATA_WHITE_KING);
            break;
        case 'Q':
            BoardSetSquare(board, file++, rank, BOARD_DATA_WHITE_QUEEN);
            break;
        case 'R':
            BoardSetSquare(board, file++, rank, BOARD_DATA_WHITE_ROOK);
            break;
        case 'B':
            BoardSetSquare(board, file++, rank, BOARD_DATA_WHITE_BISHOP);
            break;
        case 'N':
            BoardSetSquare(board, file++, rank, BOARD_DATA_WHITE_KNIGHT);
            break;
        case 'P':
            BoardSetSquare(board, file++, rank, BOARD_DATA_WHITE_PAWN);
            break;
        case 'k':
            BoardSetSquare(board, file++, rank, BOARD_DATA_BLACK_KING);
            break;
        case 'q':
            BoardSetSquare(board, file++, rank, BOARD_DATA_BLACK_QUEEN);
            break;
        case 'r':
            BoardSetSquare(board, file++, rank, BOARD_DATA_BLACK_ROOK);
            break;
        case 'b':
            BoardSetSquare(board, file++, rank, BOARD_DATA_BLACK_BISHOP);
            break;
        case 'n':
            BoardSetSquare(board, file++, rank, BOARD_DATA_BLACK_KNIGHT);
            break;
        case 'p':
            BoardSetSquare(board, file++, rank, BOARD_DATA_BLACK_PAWN);
            break;
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8': {
            int emptySquares = *pos - '0';
            if (file + emptySquares > 8)
                return PARSE_ERROR_INVALID_BOARD_STATE;
            for (int i = 0; i < emptySquares; i++) {
                BoardSetSquare(board, file++, rank, BOARD_DATA_EMPTY);
            }
        } break;
        case '/':
            if (file <= FILE_H)
                return PARSE_ERROR_INVALID_BOARD_STATE;
            file = FILE_A;
            if (rank == RANK_1)
                return PARSE_ERROR_INVALID_BOARD_STATE;
            --rank;
            break;
        default:
            return PARSE_ERROR_SYNTAX_ERROR;
        }
        ++pos;
    }

    // Skip whitespace
    while (pos < fenString + fenLength && isspace(*pos))
        ++pos;

    // Set which player is to move
    if (pos < fenString + fenLength) {
        switch (*pos) {
        case 'w':
        case 'W':
            board->PlayerMove = PLAYER_MOVE_WHITE;
            break;
        case 'b':
        case 'B':
            board->PlayerMove = PLAYER_MOVE_BLACK;
            break;
        default:
            return PARSE_ERROR_SYNTAX_ERROR;
        }
        ++pos;
    }

    // Skip whitespace
    while (pos < fenString + fenLength && isspace(*pos))
        ++pos;

    // Parse castling rights
    board->CastlingRights = 0;
    if (pos < fenString + fenLength) {
        if (*pos == '-') {
            ++pos;
        } else {
            while (pos < fenString + fenLength && !isspace(*pos)) {
                switch (*pos) {
                case 'K':
                    board->CastlingRights |= CASTLING_KINGSIDE_WHITE;
                    break;
                case 'Q':
                    board->CastlingRights |= CASTLING_QUEENSIDE_WHITE;
                    break;
                case 'k':
                    board->CastlingRights |= CASTLING_KINGSIDE_BLACK;
                    break;
                case 'q':
                    board->CastlingRights |= CASTLING_QUEENSIDE_BLACK;
                    break;
                default:
                    return PARSE_ERROR_SYNTAX_ERROR;
                }
                ++pos;
            }
        }
    }

    // Skip whitespace
    while (pos < fenString + fenLength && isspace(*pos))
        ++pos;

    // Parse en passant target square
    if (pos < fenString + fenLength) {
        if (*pos == '-') {
            ++pos;
        } else {
            // Parse en passant square (e.g. "e3")
            if (pos + 1 >= fenString + fenLength)
                return PARSE_ERROR_SYNTAX_ERROR;

            char fileChar = *pos;
            char rankChar = *(pos + 1);

            if (fileChar < 'a' || fileChar > 'h')
                return PARSE_ERROR_SYNTAX_ERROR;

            switch (rankChar) {
            case '3':
                BoardSetSquare(board, fileChar - 'a', RANK_4, BOARD_DATA_WHITE_PAWN_EP_CAPTURABLE);
                break;
            case '6':
                BoardSetSquare(board, fileChar - 'a', RANK_5, BOARD_DATA_BLACK_PAWN_EP_CAPTURABLE);
                break;
            default:
                return PARSE_ERROR_SYNTAX_ERROR;
            }

            pos += 2;
        }
    }

    // Skip whitespace
    while (pos < fenString + fenLength && isspace(*pos))
        ++pos;

    // Parse halfmove clock
    if (pos < fenString + fenLength) {
        board->HalfmoveClock = 0;
        while (pos < fenString + fenLength && isdigit(*pos)) {
            board->HalfmoveClock = board->HalfmoveClock * 10 + (*pos - '0');
            ++pos;
        }
    }

    // Skip whitespace
    while (pos < fenString + fenLength && isspace(*pos))
        ++pos;

    // Parse fullmove counter
    if (pos < fenString + fenLength) {
        board->FullmoveCounter = 0;
        while (pos < fenString + fenLength && isdigit(*pos)) {
            board->FullmoveCounter = board->FullmoveCounter * 10 + (*pos - '0');
            ++pos;
        }
        if (board->FullmoveCounter == 0)
            board->FullmoveCounter = 1;
        allFieldsParsed = true;
    }

    if (allFieldsParsed)
        return PARSE_SUCCESS;
    else
        return PARSE_ERROR_INCOMPLETE_FEN;
}

int main()
{
    Board board;

    // Test 1: Starting position
    const char* starting_fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
    printf("Test 1 - Starting position:\n");
    printf("FEN: %s\n", starting_fen);

    int result = ParseFen(starting_fen, strlen(starting_fen), &board);
    if (result == 0) {
        printf("✓ Parsing successful!\n");
        printf("Player to move: %s\n", board.PlayerMove ? "Black" : "White");
        printf("Castling rights: %d (should be 15)\n", board.CastlingRights);
        printf("White king (e1): %d (should be 1)\n", BoardGetSquare(&board, FILE_E, RANK_1));
        printf("Black king (e8): %d (should be 9)\n", BoardGetSquare(&board, FILE_E, RANK_8));
    } else {
        printf("✗ Parsing failed with error code: %d\n", result);
    }

    printf("\n");

    // Test 2: Position with en passant (black pawn moved d7-d5, ep target d6)
    const char* ep_fen = "rnbqkbnr/ppp1pppp/8/3pP3/8/8/PPPP1PPP/RNBQKBNR w KQkq d6 0 2";
    printf("Test 2 - En passant position:\n");
    printf("FEN: %s\n", ep_fen);

    result = ParseFen(ep_fen, strlen(ep_fen), &board);
    if (result == 0) {
        printf("✓ Parsing successful!\n");
        printf("Black pawn on d5: %d (should be 15 for EP capturable)\n", BoardGetSquare(&board, FILE_D, RANK_5));
        printf("En passant target d6 maps to rank: %d\n", RANK_6);
    } else {
        printf("✗ Parsing failed with error code: %d\n", result);
    }

    // Test 3: Incomplete FEN
    const char* inc_fen = "rnbqkbnr/ppp1pppp/8/3pP3/8/8/PPPP1PPP/RNBQKBNR w KQkq d6";
    printf("Test 3 Incomplete- :\n");
    printf("FEN: %s\n", inc_fen);

    result = ParseFen(inc_fen, strlen(inc_fen), &board);
    if (result == 0) {
        printf("✓ Parsing successful!\n");
    } else {
        printf("✗ Parsing failed with error code: %d\n", result);
    }
    return 0;
}
