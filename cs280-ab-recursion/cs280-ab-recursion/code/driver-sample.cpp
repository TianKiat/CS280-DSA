#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <iostream>
#include <iomanip>
#include <stddef.h>

#if defined(__GNUC__)
#include <unistd.h>
#include <getopt.h>
#else
#include "getopt.h"
#endif

#include "Sudoku.h"

  // TODO: allocate dynamically
const int MAX_BOARD_SIZE = 10000;

  // These values control the amount of output
namespace
{
  bool SHOW_BOARDS = false;
  size_t SHOW_BOARDS_EVERY = 1;
  bool CLEAR_SCREEN = false;
  bool SHOW_MESSAGES = false;
  bool PAUSE = false;
  bool SHOW_SOLUTION = true;
  bool ABORTED = false;
  bool SHOW_STATS = false;
}

char gFile[FILENAME_MAX];
char *gBoard = 0;
int BASESIZE = 3;
Sudoku::SymbolType SYMBOL_TYPE = Sudoku::SYM_NUMBER;

void usage(void)
{
  std::cout << "Usage: sudoku [options] input_file\n";
  std::cout << "\n";
  std::cout << "Options:\n";
  std::cout << " -a  --show_solution=X       show final board (0-OFF, 1-ON, default is ON).\n";
  std::cout << " -b  --basesize=X            the size of the board (e.g. 3, 4, 5, etc. Default is 3)\n";
  std::cout << " -c  --clear_screen          clear the screen between boards.\n";
  std::cout << " -e  --show_boards_every=X   only show boards after every X iterations.\n";
  std::cout << " -h  --help                  display this information.\n";
  std::cout << " -m  --show_messages         show all messages (placing, removing, backtracking, etc.\n";
  std::cout << " -p  --pause                 pause between moves (press a key to continue).\n";
  std::cout << " -s  --show_boards           show boards after placing/removing.\n";
  std::cout << " -t  --symbol_type=X         the type of symbols to use, numbers or letters (0 is numbers, 1 is letters)\n";
  std::cout << " -w  --show_stats=X          show statistics after each move.\n";
  std::cout << "\n";
}

void dump_options(void)
{
  std::cout << "     show_solution: " << (SHOW_SOLUTION ? "true" : "false") << std::endl;
  std::cout << "          basesize: " << BASESIZE << std::endl;
  std::cout << "      clear_screen: " << (CLEAR_SCREEN ? "true" : "false") << std::endl;
  std::cout << " show_boards_every: " << SHOW_BOARDS_EVERY << std::endl;
  std::cout << "     show_messages: " << (SHOW_MESSAGES ? "true" : "false") << std::endl;
  std::cout << "             pause: " << (PAUSE ? "true" : "false") << std::endl;
  std::cout << "       show_boards: " << (SHOW_BOARDS ? "true" : "false") << std::endl;
  std::cout << "       symbol_type: " << (SYMBOL_TYPE == Sudoku::SYM_NUMBER ? "Numbers" : "Letters") << std::endl;
  std::cout << "        show_stats: " << (SHOW_STATS ? "true" : "false") << std::endl;
  std::cout << "          filename: " << gFile << std::endl;
}

void parse_commandline(int argc, char **argv)
{
  int fail = 0;
  int option_index = 0;
  
  static struct option long_options[] = 
  {

    {"show_solution",     required_argument, NULL, 'a'},
    {"basesize",          required_argument, NULL, 'b'},
    {"clear_screen",      no_argument,       NULL, 'c'},
    {"show_boards_every", required_argument, NULL, 'e'},
    {"help",              no_argument,       NULL, 'h'},
    {"show_messages",     no_argument,       NULL, 'm'},
    {"pause",             no_argument,       NULL, 'p'},
    {"show_boards",       no_argument,       NULL, 's'},
    {"symbol_type",       required_argument, NULL, 't'},
    {"show_stats",        no_argument,       NULL, 'w'},
    {NULL,                 0,                NULL, 0}
  };

  optind = 1;
  for(;;)
  {
    int opt = getopt_long(argc, argv, "-:a:b:ce:hmpst:w", long_options, &option_index);
    
    if (opt == -1)
      break;

    switch (opt) 
    {
      case 0: /* long option */
        /*
        printf("long option %s", long_options[option_index].name);
        if (optarg)
           printf(" with arg %s", optarg);
        printf("\n");
        */
        break;

      case 'a':
        if (std::atoi(optarg))
          SHOW_SOLUTION = true;
        break;
      case 'b':
        BASESIZE = std::atoi(optarg);
        break;
      case 'c':
        CLEAR_SCREEN = true;
        break;
      case 'e':
        SHOW_BOARDS_EVERY = std::atoi(optarg);
        break;
      case 'h':
        usage();
        std::exit(0);
      case 'm':
        SHOW_MESSAGES = true;
        break;
      case 'p':
        PAUSE = true;
        break;
      case 's':
        SHOW_BOARDS = true;
        break;
      case 't':
        if (std::atoi(optarg))
          SYMBOL_TYPE = Sudoku::SYM_LETTER;
        else
          SYMBOL_TYPE = Sudoku::SYM_NUMBER;
        break;
      case 'w':
        SHOW_STATS = true;
        break;
      case '?':
        std::cout << "Unknown option: " << optopt << std::endl;
        fail = 1;
        break;
      case ':':
        std::cout << "Missing option for " << optopt << std::endl;
        fail = 1;
        break;
      case 1:
        std::strcpy(gFile, optarg);
        break;
    }
  }

  if (fail)
  {
    usage();
    std::exit(-1);
  }
}

void PrintStats(const Sudoku& board)
{
  Sudoku::SudokuStats stats = board.GetStats();
  std::cout << "  Basesize: " << stats.basesize << std::endl;
  std::cout << "     Moves: " << stats.moves << std::endl;
  std::cout << "    Placed: " << stats.placed << std::endl;
  std::cout << "Backtracks: " << stats.backtracks << std::endl;
  std::cout << "  Filename: " << gFile << std::endl;
}

void DumpBoard(const char *board, unsigned basesize, size_t moves)
{
  unsigned blocksize = basesize * basesize;
  std::cout << "Moves: " << moves << std::endl;

    // two spaces for each symbol
  int len = (blocksize * 2) + (basesize * 2) + 1;

    // print top horizontal line with - and + in the appropriate locations
  for (int k = 0; k < len; k++)
  {
    if ( (k == 0) || !(k % (2 * basesize + 2)))
      std::cout << "+";
    else
      std::cout << "-";
  }
  std::cout << std::endl;

  int count = 0;
  for (unsigned i = 0; i < blocksize * blocksize - 1; i += blocksize)
  {
    std::cout << "|";
    for (unsigned j = 0; j < basesize; j++)
    {
      for (unsigned k = 0; k < basesize; k++)
      {
        std::cout << std::setw(2) << board[i + j * basesize + k + 0];
      }
      std::cout << " |";
      //if (j && ((j == 2) || (j == 5) || (j == 8)) )
    }
    std::cout << std::endl;

    count++;

      // print horizontal lines with - and + in the appropriate locations
      // after each 'set' of symbols
    if (!(count % basesize))
    {
      for (int k = 0; k < len; k++)
      {
        if ( (k == 0) || !(k % (2 * basesize + 2)))
          std::cout << "+";
        else
          std::cout << "-";
      }
      std::cout << std::endl;
    }
  }
}

void Pause(void)
{
  if (ABORTED)
    return;

  if (std::fgetc(stdin) == 'x')
  {
    std::cout << "User abort.\n";
    ABORTED = true;
    //std::exit(1);
  }
}

struct RowCol
{
  unsigned row;
  unsigned col;
};

unsigned index_to_row(unsigned index, unsigned basesize)
{
  return index / (basesize * basesize);
}

unsigned index_to_col(unsigned index, unsigned basesize)
{
  return index % (basesize * basesize);
}


RowCol IndexToRowCol(unsigned index, unsigned basesize)
{
  RowCol rc;
  rc.row = index_to_row(index, basesize);
  rc.col = index_to_col(index, basesize);
  return rc;
}

bool Callback(const Sudoku& sudoku, const char *board, Sudoku::MessageType message, 
              size_t move, unsigned basesize, unsigned index, char symbol)
{
  if (CLEAR_SCREEN && message != Sudoku::MSG_ABORT_CHECK)
    #if defined (__GNUC__)
    if (std::system("clear"))
    #else
    if (std::system("cls"))
    #endif
      std::exit(0);

  switch (message)
  {
    case Sudoku::MSG_STARTING:
      if (SHOW_MESSAGES)
        std::cout << "Starting... \n";
      DumpBoard(board, basesize, move);
  //    if (SHOW_STATS)
        PrintStats(sudoku);
      std::cout << std::endl;
    break;

    case Sudoku::MSG_PLACING:
      if (SHOW_MESSAGES)
      {
        RowCol rc = IndexToRowCol(index, basesize);
        std::cout << "Move: " << move << ", Placing " << symbol << " at " << rc.row + 1 << "," << rc.col + 1 << " (cell: " << index + 1 << ").\n";
      }
      if (SHOW_BOARDS)
      {
        if (!(move % SHOW_BOARDS_EVERY))
        {
          DumpBoard(board, basesize, move);
          if (SHOW_STATS)
            PrintStats(sudoku);
          std::cout << std::endl;
        }
      }
      break;

    case Sudoku::MSG_REMOVING:
      if (SHOW_MESSAGES)
      {
        RowCol rc = IndexToRowCol(index, basesize);
        std::cout << "Move: " << move << ", Removing " << symbol << " from " << rc.row + 1 << "," << rc.col + 1 << " (cell: " << index + 1 << ").\n";
      }
      if (SHOW_BOARDS)
      {
        if (!(move % SHOW_BOARDS_EVERY))
        {
          DumpBoard(board, basesize, move);
          if (SHOW_STATS)
            PrintStats(sudoku);
          std::cout << std::endl;
        }
      }
      break;

    case Sudoku::MSG_FINISHED_OK:
      std::cout << "Solution found in " << move << " moves.\n";
      if (SHOW_SOLUTION)
        DumpBoard(board, basesize, move);
      break;

    case Sudoku::MSG_FINISHED_FAIL:
      std::cout << "No solution found after " << move << " moves\n";
      break;

    case Sudoku::MSG_ABORT_CHECK:
      if (ABORTED)
        return true;
      break;

    default:
      std::cout << "Unknown message.\n";
  }

  if (PAUSE)
    Pause();

  return false;
}

void stripnl(char *line)
{
  size_t len = std::strlen(line);
  line[len - 1] = 0;
}

void acquire_board(const char *filename)
{
  std::FILE *fp = std::fopen(filename, "rt");
  if (!fp)
  {
    std::cout << "Failed to open " << filename << " for input.\n";
    std::exit(-1);
  }

    // TODO: Allocate dynamically 
  char buffer[MAX_BOARD_SIZE];
  while (!std::feof(fp))
  {
    if (std::fgets(buffer, MAX_BOARD_SIZE, fp) == NULL)
      break;

      // skip comments (if any)
    if (buffer[0] == '#')
      continue;

    stripnl(buffer);
    gBoard = new char[strlen(buffer) + 1];
    std::strcpy(gBoard, buffer);

      // Currently, there is only 1 board per file
    break;
  }
  std::fclose(fp);
}

int main()//int argc, char **argv)
{
 /*char test[20];
  std::cin>>test;
  std::cout<<test<<"\nhere";
 char **test_input = &test;
 std::cout<<"here";
 char** test = new char*;   

  std::cin>>*test; 
  std::cout<<**test;*/
  /*for (int i = 0; i < argc; ++i) 
        std::cout <<argc<< argv[i] << "\n"; */
  char test[20];
  std::cin>> test;
 //parse_commandline(argc,argv);
  //dump_options();
  /*if (!std::strlen(gFile))
  {
    usage();
    std::exit(1);
  }*/

  // updating BASESIZE
  char *search = std::strchr(test,'4');
  if(search !=NULL)
     BASESIZE = 4;
  search = std::strchr(test,'5'); 
  if(search !=NULL)
     BASESIZE = 5;
  
 if(std::strcmp(test,"board3-2.txt") == 0 || std::strcmp(test,"board4-1.txt") == 0 || std::strcmp(test,"board4-2.txt") == 0 || std::strcmp(test,"board4-3.txt") == 0 || std::strcmp(test,"board4-4.txt") == 0 || std::strcmp(test,"board5-1.txt") == 0 || std::strcmp(test,"board5-2.txt") == 0)
 {
     SYMBOL_TYPE = Sudoku::SYM_LETTER;
 } 
  std::strcpy(gFile,test);
  acquire_board(gFile);
  Sudoku board(BASESIZE, SYMBOL_TYPE, Callback);
  board.SetupBoard(gBoard, std::strlen(gBoard));
  board.Solve();
  PrintStats(board);

  std::cout << std::endl;

  delete [] gBoard;

  return 0;
}
