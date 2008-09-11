#ifndef __octree_cursor
#define __octree_cursor
// Included by octree

/**\brief An octree cursor.
  *
  * Unlike an iterator, an octree cursor does not provide an order in which to traverse nodes.
  * Instead, it provides 2 primitive operations: up() and down() that can be used to
  * traverse the tree in any order.
  * There are also some utility routines named over(), axis_partner(), and neighbor() that
  * call up() followed by down() with specific arguments to help traverse nodes that are
  * children of the same parent.
  *
  * A cursor contains no storage beyond its base class, octree_path, so you may assign to a
  * cursor from any descendant of octree_path including octree_iterator.
  */
template< typename _T, typename _R, typename _P, typename _O, typename _OP, int _d = 3 >
class octree_cursor : public octree_path<_T,_R,_P,_O,_OP,_d>
{
public:
  typedef _O octree_type;
  typedef _OP octree_pointer;
  typedef typename _O::allocator_type octree_allocator_type;
  typedef typename _O::octree_node_reference octree_node_reference;
  typedef typename _O::octree_node_pointer octree_node_pointer;

  typedef octree_path< _T, _T&, _T*, _O, _O*, _d > path;
  typedef octree_path< _T, const _T&, const _T*, _O, const _O*, _d > const_path;
  typedef octree_path< _T, _R, _P, _O, _OP, _d > self_path;

  typedef octree_cursor< _T, _T&, _T*, _O, _O*, _d > cursor;
  typedef octree_cursor< _T, const _T&, const _T*, _O, const _O*, _d > const_cursor;
  typedef octree_cursor< _T, _R, _P, _O, _OP, _d > self_cursor;

  octree_cursor();
  octree_cursor( octree_pointer otree );
  octree_cursor( const const_path& src );
  ~octree_cursor();

  void up();
  void down( int child_of_this_node );

  int where() const;

  void over( int child_of_shared_parent );
  void axis_partner( int axis );
  bool axis_bit( int axis ) const;

  bool visit( const vtkstd::vector<int>& path );

  virtual self_path& operator = ( const path& it );
  virtual self_path& operator = ( const const_path& it );
};

#endif // __octree_cursor
