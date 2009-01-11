#ifndef __octree_path
#define __octree_path

// Included by octree

/**\brief An octree path.
  *
  * A path is like an iterator without the capability to perform linear traversal.
  */
template< typename T_, typename R_, typename P_, typename O_, typename OP_, int d_ = 3 >
class octree_path
{
public:
  typedef O_ octree_type;
  typedef OP_ octree_pointer;
  typedef typename O_::allocator_type octree_allocator_type;
  typedef typename O_::octree_node_reference octree_node_reference;
  typedef typename O_::octree_node_pointer octree_node_pointer;
  typedef typename vtkstd::vector<octree_node_pointer>::size_type size_type;

  typedef octree_path< T_, T_&, T_*, O_, O_*, d_ > path;
  typedef octree_path< T_, const T_&, const T_*, O_, const O_*, d_ > const_path;
  typedef octree_path< T_, R_, P_, O_, OP_, d_ > self_path;

  octree_node_pointer _M_root;                    // The root of the octree we are iterating over
  vtkstd::vector<octree_node_pointer> _M_parents; // List of parent nodes
  vtkstd::vector<int> _M_indices;                 // List of parent child indices
  octree_node_pointer _M_current_node;            // Current path head

  octree_path();
  octree_path( octree_pointer otree );
  octree_path( octree_node_pointer oroot );
  octree_path( octree_node_pointer oroot, vtkstd::vector<int>& children );
  virtual ~octree_path();

  octree_node_reference operator * () const { return *_M_current_node; }
  octree_node_pointer operator -> () const { return &(operator*()); }

  size_type level() const { return this->_M_parents.size(); }

  self_path& operator = ( const path& it );
#if ! ( defined(_MSC_VER) && (_MSC_VER < 1300) )
  self_path& operator = ( const const_path& src );
#endif

  bool operator == ( const path& it ) { return _M_root == it._M_root && _M_current_node == it._M_current_node; }
#if ! ( defined(_MSC_VER) && (_MSC_VER < 1300) )
  bool operator == ( const const_path& it ) { return _M_root == it._M_root && _M_current_node == it._M_current_node; }
#endif

  bool operator != ( const path& it ) { return _M_root != it._M_root || _M_current_node != it._M_current_node; }
#if ! ( defined(_MSC_VER) && (_MSC_VER < 1300) )
  bool operator != ( const const_path& it ) { return _M_root != it._M_root || _M_current_node != it._M_current_node; }
#endif
};

#endif // __octree_path
