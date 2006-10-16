#include "freerange"
#include "vtkIOStream.h"

// To test freerange insertion and deletion, we use an action
// table. Each triple of integers in the list below corresponds
// to a call that will be made on a freerange instance. When the
// first number is 0, grab() will be called. When nonzero, free()
// will be called.
// When calling grab() the second number serves as the number of
// entries and the third is the expected return value -- an error
// message is printed if it is not matched.
// When calling free() the second number servers as the starting
// index of the item to be freed and the third number is the
// number of items to be freed.
static int actions[] = {
  0,  1,  0,
  0,  5,  1,
  0, 10,  6, /* test case where initial size aligns with entry bdy */
  1,  1,  8,
  0,  8,  1, /* test that we get back the hole we opened */
  0,  0, -1, /* test that empty grab returns invalid result */
  0,  1, 16,
  1, 14,  2, /* test free near (but not at end) of list */
  0,  2, 14, /* if the above worked, we get the same item back */
  1, 13,  3,
  1, 16,  1, /* what happens when we free the end and there's a hole nearby? */
  0, 17, 16, /* for freerange (not freelist), the holes remain. This also tests an entry that would overlap allocated bdy. */
 -1, -1, -1  /* signal end of test */
};

int testInsertionDeletion( int argc, char* argv[] )
{
  freerange<int,int,-5> fr;
  int idx;
  int count = 0;
  int grab = 0;
  int* action = actions;
  int result = 0;

  while ( *action != -1 )
    {
    switch ( *action )
      {
    case 0:
      idx = fr.grab( action[1] );
      count += action[1];
      if ( idx != action[2] )
        {
        cerr << "Grab " << grab << " returned " << idx
          << ", was expecting " << action[2] << endl;
        result = 1;
        }
      break;
    case 1:
      fr.free( action[1], action[2] );
      count -= action[2];
      break;
    case -1:
    default:
      break;
      }
    action += 3;
    ++grab;
    }

  if ( fr.size() != count )
    {
    cerr << "Entries in use was " << fr.size()
      << " but expected " << count << endl;
    result = 1;
    }

  return result;
}
