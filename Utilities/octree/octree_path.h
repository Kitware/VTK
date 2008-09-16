#ifndef __octree_path
#define __octree_path

// Included by octree

/**\brief An octree path.
  *
  * A path is like an iterator without the capability to perform linear traversal.
  */
template< typename _T, typename _R, typename _P, typename _O, typename _OP, int _d = 3 >
class octree_path
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

  octree_pointer _M_octree;                    // The octree we are iterating over
  vtkstd::vector<octree_node_pointer> _M_parents; // List of parent nodes
  vtkstd::vector<int> _M_indices;                 // List of parent child indices
  octree_node_pointer _M_current_node;         // Current path head

  octree_path();
  octree_path( octree_pointer otree );
  octree_path( octree_pointer otree, vtkstd::vector<int>& children );
  virtual ~octree_path();

  octree_node_reference operator * () const { return *_M_current_node; }
  octree_node_pointer operator -> () const { return &(operator*()); }

  int level() const { return this->_M_parents.size(); }

  self_path& operator = ( const path& it );
#if ! ( defined(_MSC_VER) && (_MSC_VER < 1300) )
  self_path& operator = ( const const_path& src );
#endif

  bool operator == ( const path& it ) { return _M_octree == it._M_octree && _M_current_node == it._M_current_node; }
#if ! ( defined(_MSC_VER) && (_MSC_VER < 1300) )
  bool operator == ( const const_path& it ) { return _M_octree == it._M_octree && _M_current_node == it._M_current_node; }
#endif

  bool operator != ( const path& it ) { return _M_octree != it._M_octree || _M_current_node != it._M_current_node; }
#if ! ( defined(_MSC_VER) && (_MSC_VER < 1300) )
  bool operator != ( const const_path& it ) { return _M_octree != it._M_octree || _M_current_node != it._M_current_node; }
#endif
};

#endif // __octree_path
