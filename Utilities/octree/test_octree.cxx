#include <octree>

#include <iostream>

int main()
{
  // Construct a small 2-d binary tree.
  double center[2] = { 0.5, 0.5 };
  octree<int,2> foo( center, 1. );
  foo.root()->add_children();
  foo.root()->value() = 42;
  *(*foo.root())[0] = 25;
  *(*foo.root())[1] = 19;
  (*foo.root())[1].add_children();
  (*foo.root())[3].add_children();
  *(*foo.root())[1][0] = 38;
  *(*foo.root())[1][1] = 5;
  *(*foo.root())[1][2] = -19;
  *(*foo.root())[1][3] = 1;
  *(*foo.root())[2] = 8;
  *(*foo.root())[3] = 3;
  *(*foo.root())[3][0] = 15;
  (*foo.root())[3].remove_children();

  cout << "Root is " << foo.root()->value() << "\n";
  cout << "Child 0 is " << (*foo.root())[0].value() << "\n";
  cout << "Child 1 is " << (*foo.root())[1].value() << "\n";

  // Now test an iterator
  octree<int,2>::iterator it;
  for ( it = foo.begin(); it != foo.end(); ++it )
    {
    /*
    const double* bds = it->center();
    double he = it->size() / 2.;
    */
    cout
      << "Node  0x" << hex << (&*it)
      << " (" << (it.level()) << ") "
      << " = " << it->value()
      /*
      << " [" << (bds[0] - he) << "->" << (bds[0] + he)
      << ", " << (bds[1] - he) << "->" << (bds[1] + he) << "]"
      */
      << "\n";
    }

  cout << "\n\n";

  for ( it = foo.begin( false ); it != foo.end( false ); ++it )
    {
    /*
    const double* bds = it->center();
    double he = it->size() / 2.;
    */
    cout
      << "Node  0x" << hex << (&*it)
      << " (" << (it.level()) << ") "
      << " = " << it->value()
      /*
      << " [" << (bds[0] - he) << "->" << (bds[0] + he)
      << ", " << (bds[1] - he) << "->" << (bds[1] + he) << "]"
      */
      << "\n";
    }

  cout << "\n\n";

  it = foo.end();
  do
    {
    --it;
    cout
      << "Node  0x" << hex << (&*it)
      << " (" << (it.level()) << ") "
      << " = " << it->value() << "\n";
    }
  while ( it != foo.begin() );

  cout << "\n\n";

  it = foo.end( false );
  do
    {
    --it;
    cout
      << "Node  0x" << hex << (&*it)
      << " (" << (it.level()) << ") "
      << " = " << it->value() << "\n";
    }
  while ( it != foo.begin( false ) );

  cout << "\n\n";

  // Now test the "immediate family" mode.
  it = foo.begin();
  ++it;
  it.immediate_family( true );
  for ( ; it != foo.end(); ++it )
    {
    /*
    const double* bds = it->center();
    double he = it->size() / 2.;
    */
    cout
      << "Node  0x" << hex << (&*it)
      << " (" << (it.level()) << ") "
      << " = " << it->value()
      /*
      << " [" << (bds[0] - he) << "->" << (bds[0] + he)
      << ", " << (bds[1] - he) << "->" << (bds[1] + he) << "]"
      */
      << "\n";
    }

  cout << "\n\n";

  it = foo.begin( false );
  ++it;
  it.immediate_family( true );
  for ( ; it != foo.end( false ); ++it )
    {
    /*
    const double* bds = it->center();
    double he = it->size() / 2.;
    */
    cout
      << "Node  0x" << hex << (&*it)
      << " (" << (it.level()) << ") "
      << " = " << it->value()
      /*
      << " [" << (bds[0] - he) << "->" << (bds[0] + he)
      << ", " << (bds[1] - he) << "->" << (bds[1] + he) << "]"
      */
      << "\n";
    }

  cout << "\n\n";

  // Test octree cursors:
  octree<int,2>::cursor curs( &foo );
  curs.down( 0 );
  curs.over( 1 );
  cout << "Initial L2Node: " << "level " << curs.level() << " where " << curs.where() << " val " << curs->value() << "\n";
  curs.axis_partner( 1 );
  cout << "Axis 1 partner: " << "level " << curs.level() << " where " << curs.where() << " val " << curs->value() << "\n";
  curs.over( 1 );
  curs.axis_partner( 0 );
  cout << "Axis 0 partner: " << "level " << curs.level() << " where " << curs.where() << " val " << curs->value() << "\n";
  curs.over( 1 );
  curs.down( 3 );
  cout << "Down to level2: " << "level " << curs.level() << " where " << curs.where() << " val " << curs->value() << "\n";

  // Copy an iterator's position
  curs = foo.begin();
  cout << "level " << curs.level() << " where " << curs.where() << " val " << curs->value() << "\n";

  return 0;
}
