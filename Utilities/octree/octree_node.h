#ifndef __octree_node
#define __octree_node

#include <iterator>

template< typename _T, int _d, typename _A > class octree;

/**\brief An n-dimensional octree node.
  *
  * Each child node is indexed by an integer whose first _d bits describe the node's position in its parent node.
  * A bit value of 0 means the node is to the on the lower side of the hyperplane bisecting the corresponding axis.
  * There are \f$2^{\mathrm{\texttt{\_d}}}\f$ child nodes foreach non-leaf node.
  * As an example, consider a 3-dimensional octree (coordinate axes x, y, and z) with a child node index of 5.
  * The value 5 corresponds to a bit vector of 101, which indicates that node 5 is on the +x, -y, and +z sides
  * of the x, y, and z (respectively) planes bisecting the parent.
  *
  * Octree nodes store application data, a list of pointers to child nodes (or NULL), and a pointer to the octree containing them.
  * Octree nodes do not store a pointer to their direct parent, their bounding box, or any information describing the path up
  * to the root node; this information is all available in the iterator.
  */
template< typename _T, int _d = 3, typename _A = vtkstd::allocator<_T> >
struct octree_node
{
  typedef octree<_T,_d,_A>* octree_pointer;
  typedef octree_node<_T,_d,_A>* octree_node_pointer;
  typedef const octree_node<_T,_d,_A>* const_octree_node_pointer;
  typedef octree_node<_T,_d,_A>& octree_node_reference;
  typedef const octree_node<_T,_d,_A>& const_octree_node_reference;
  typedef _T value_type;
  typedef _T* pointer;
  typedef _T& reference;
  typedef const _T* const_pointer;
  typedef const _T& const_reference;

  octree_pointer _M_parent;
  octree_node_pointer _M_children;
  double _M_center[_d];
  double _M_size;
  value_type _M_data;

  octree_node();
  octree_node( octree_pointer parent, const double* center, double size );
  octree_node( octree_pointer parent, const double* center, double size, const value_type& data );
  octree_node( octree_node_pointer parent, int which, const value_type& data );
  ~octree_node();

  bool is_leaf_node() { return this->_M_children == 0; }
  int num_children() { return this->_M_children ? (1<<_d) : 0; }
  bool add_children();
  bool add_children( const _T& child_initializer );
  bool remove_children();

  const double* center() const { return this->_M_center; }
  double size() const { return this->_M_size; }

  reference value() { return this->_M_data; }
  const reference value() const { return this->_M_data; }

  const_octree_node_reference operator [] ( int child ) const;
  octree_node_reference operator [] ( int child );

  reference operator * () { return _M_data; }
  const_reference operator * () const { return _M_data; }
};

#endif // __octree_node
