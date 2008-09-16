#ifndef __octree_iterator
#define __octree_iterator

// Included by octree

#include <iterator>

/**\brief An octree iterator.
  *
  * Unlike most STL container iterators, octree iterators contain some state that most developers will be interested in.
  * Specifically, the iterator may be set to traverse siblings but not children, only leaf nodes, or all nodes in an octree.
  * Also, the bounding box of an octree node is implicit in its position in the hierarchy;
  * since octree nodes themselves do not contain their position relative to their parents it is up to the iterator
  * to provide bounding box information.
  *
  *\bug Const octree iterators don't seem to work -- comparison operators aren't defined properly?
  */
template< typename _T, typename _R, typename _P, typename _O, typename _OP, int _d = 3 >
class octree_iterator : public octree_path<_T,_R,_P,_O,_OP,_d>
{
public:
  typedef _O octree_type;
  typedef _OP octree_pointer;
  typedef typename _O::allocator_type octree_allocator_type;
  typedef typename _O::octree_node_reference octree_node_reference;
  typedef typename _O::octree_node_pointer octree_node_pointer;

  typedef vtkstd::bidirectional_iterator_tag iterator_category;

  typedef octree_iterator< _T, _T&, _T*, _O, _O*, _d > iterator;
  typedef octree_iterator< _T, const _T&, const _T*, _O, const _O*, _d > const_iterator;
  typedef octree_iterator< _T, _R, _P, _O, _OP, _d > self_iterator;

  bool _M_immediate_family;                 // Iterate over all the subnodes or just the direct children?
  bool _M_only_leaf_nodes;                  // Should the iterator visit all nodes or only leaf nodes?

  octree_iterator();
  octree_iterator( octree_pointer otree, octree_node_pointer onode, bool only_leaves = true );
  octree_iterator( const const_iterator& it );
  ~octree_iterator();

  octree_node_pointer check_incr();
  octree_node_pointer check_decr();
  void immediate_family( bool state );
  bool immediate_family() const { return this->_M_immediate_family; }
  bool& immediate_family() { return this->_M_immediate_family; }

  virtual self_iterator& operator = ( const iterator& it );
#if ! ( defined(_MSC_VER) && (_MSC_VER < 1300) )
  virtual self_iterator& operator = ( const const_iterator& it );
#endif

  self_iterator& operator ++ ()      { this->_M_current_node = check_incr(); return *this; }
  self_iterator  operator ++ ( int ) { self_iterator tmp = *this; this->_M_current_node = check_incr(); return tmp; }

  self_iterator& operator -- ()      { this->_M_current_node = check_decr(); return *this; }
  self_iterator  operator -- ( int ) { self_iterator tmp = *this; this->_M_current_node = check_decr(); return tmp; }

};

#endif // __octree_iterator
