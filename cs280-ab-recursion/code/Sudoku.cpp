/******************************************************************************/
/*!
\file   Sudoku.cpp
\author Ng Tian Kiat
\par    email: tiankiat.ng\@digipen.edu
\par    Course: CS280
\par    Assignment bonus
\date   19 March 2021
\brief  
  This file contains the definitions for the Sudoku.
*/
/******************************************************************************/
#include "Sudoku.h"

/******************************************************************************/
/*!
\brief
  This is the constructor for a Sudoku.
\param basesize, the width of a box.
\param stype, the symbol to use for the sudoku, alphabets or numbers.
\param callback, a callback defined by a client used for tracking steps.
*/
/******************************************************************************/
Sudoku::Sudoku(int basesize, SymbolType stype, SUDOKU_CALLBACK callback)
    : symbol_t{stype}, cb{callback}
{
  length = basesize * basesize;
  stats.basesize = basesize;
  board = new char[length * length];
}

/******************************************************************************/
/*!
\brief
  This is the destructor for a Sudoku.
*/
/******************************************************************************/
Sudoku::~Sudoku()
{
  delete[] board;
}

/******************************************************************************/
/*!
\brief
  This functions sets up the starting board based on a string of values.
\param values, a string of values representing the start state.
\param size the size of the board.
*/
/******************************************************************************/
void Sudoku::SetupBoard(const char *values, size_t size)
{
  for (size_t i = 0; i < size; ++i)
    board[i] = values[i] == '.' ? EMPTY_CHAR : values[i];
}

/******************************************************************************/
/*!
\brief
  This function is called by the client to solve a this sudoku.
*/
/******************************************************************************/
void Sudoku::Solve()
{
  unsigned x = 0;
  unsigned y = 0;

  cb(*this, board, MessageType::MSG_STARTING, stats.moves, stats.basesize, -1, 0);

  if (place_value(x, y))
    cb(*this, board, MessageType::MSG_FINISHED_OK, stats.moves, stats.basesize, -1, 0);
  else
    cb(*this, board, MessageType::MSG_FINISHED_FAIL, stats.moves, stats.basesize, -1, 0);
}

/******************************************************************************/
/*!
\brief
  Getter for board.
\return board.
*/
/******************************************************************************/
const char *Sudoku::GetBoard() const
{
  return board;
}

/******************************************************************************/
/*!
\brief
  Getter for stats.
\return stats.
*/
/******************************************************************************/
Sudoku::SudokuStats Sudoku::GetStats() const
{
  return stats;
}

/******************************************************************************/
/*!
\brief
  This function tries to place a value at a position on the board.
\param x, x coord of the position.
\param y, y coord of the position.
\return this function returns true if a value is placed else false.
*/
/******************************************************************************/
bool Sudoku::place_value(unsigned x, unsigned y)
{
  auto index = static_cast<unsigned>(x + y * length);

  if (board[index] == EMPTY_CHAR)
  {
    char val = (symbol_t == SYM_NUMBER) ? '1' : 'A';
    for (size_t i = 0; i < length; ++i)
    {
      board[index] = val;
      ++stats.moves;
      ++stats.placed;

      cb(*this, board, MessageType::MSG_PLACING, stats.moves, stats.basesize, index, val);

      if (CheckValidMove(x, y, val))
      {
        if (index == length * length - 1)
          return true;
        else
        {
          auto next_x = x + 1;
          auto next_y = y;

          if (next_x > length - 1)
          {
            next_x = 0;
            ++next_y;
          }

          if (place_value(next_x, next_y))
            return true;
        }
      }
      else
      {
        board[index] = EMPTY_CHAR;
        --stats.placed;
        cb(*this, board, MessageType::MSG_REMOVING, stats.moves, stats.basesize, index, val);
      }

      ++val;
    }

    board[index] = EMPTY_CHAR;
    ++stats.backtracks;
    --stats.placed;
    cb(*this, board, MessageType::MSG_REMOVING, stats.moves, stats.basesize, index, val);
  }
  else
  {
    if (index == length * length - 1)
      return true;

    auto next_x = x + 1;
    auto next_y = y;

    if (next_x > length - 1)
    {
      next_x = 0;
      ++next_y;
    }

    if (place_value(next_x, next_y))
      return true;
  }

  return false;
}

/******************************************************************************/
/*!
\brief
  This function checks if a placement is valid for a given value.
\param x, x coord of the position.
\param y, y coord of the position.
\param val, value of the placement.
\return this function returns true if a value placed is valid.
*/
/******************************************************************************/
bool Sudoku::CheckValidMove(unsigned x, unsigned y, char val)
{
  auto index = x + length * y;

  for (size_t i = 0; i < length; ++i)
  {

    auto row_index = i + length * y;
    auto col_index = x + length * i;

    if ((index != row_index) && (val == board[row_index]))
      return false;

    if ((index != col_index) && (val == board[col_index]))
      return false;
  }

  auto max_width = static_cast<unsigned>(stats.basesize - 1);
  while (x > max_width)
    max_width += stats.basesize;

  auto max_height = static_cast<unsigned>(stats.basesize - 1);
  while (y > max_height)
    max_height += stats.basesize;

  auto min_width = max_width - stats.basesize + 1;
  auto min_height = max_height - stats.basesize + 1;

  for (auto i = min_height; i <= max_height; ++i)
  {
    for (auto j = min_width; j <= max_width; ++j)
    {
      auto cur_index = j + length * i;
      if (i == y || j == x)
        continue;

      if (board[cur_index] == val)
        return false;
    }
  }
  return true;
}
