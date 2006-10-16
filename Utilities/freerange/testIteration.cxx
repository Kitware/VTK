#include "freerange"
#include "vtkIOStream.h"
#include "vtkSystemIncludes.h"

// A list of entries to free before iterating
static int flist[] = {
   0, 13,
  32,  5,
  49, 11,
  64,  1,
  65,  1,
 -1,  -1
};

static int itlist[] = {
  13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30,
  31, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 60, 61, 62, 63, -1
};

int testIteration( int argc, char* argv[] )
{
  freerange<vtkTypeInt64,int> fr;
  vtkTypeInt64 fill = 500; // value to fill in allocated entries
  int i;
  int result = 0;

  for ( i = 0; i < 11; ++i )
    {
    for ( int j = 0; j <= i; ++j )
      {
      fr.grabAndAssign( fill );
      fill += 1;
      }
    fill += 3;
    }

  int* fi = flist;
  while ( *fi >= 0 )
    {
    fr.free( fi[0], fi[1] );
    fi += 2;
    }

  freerange<vtkTypeInt64,int>::iterator it;
  i = 0;
  for ( it = fr.begin(); it != fr.end() && itlist[i] >= 0; ++it, ++i )
    {
    if ( it.position() != itlist[i] )
      {
      cerr << "Expecting position to be " << itlist[i]
        << ", got " << it.position() << " instead" << endl;
      result = 1;
      }
    }
  return result;
}

