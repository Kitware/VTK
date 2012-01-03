#ifndef __octree_node
#define __octree_node

#include <iterator>

template< typename T_, int d_, typename A_ > class octree;

/**\brief An n-dimensional octree node.
  *
  * Each child node is indexed by an integer whose first d_ bits describe the node's position in its parent node.
  * A bit value of 0 means the node is to the on the lower side of the hyperplane bisecting the corresponding axis.
  * There are \f$2^{\mathrm{\texttt{\d_}}}\f$ child nodes foreach non-leaf node.
  * As an example, consider a 3-dimensional octree (coordinate axes x, y, and z) with a child node index of 5.
  * The value 5 corresponds to a bit vector of 101, which indicates that node 5 is on the +x, -y, and +z sides
  * of the x, y, and z (respectively) planes bisecting the parent.
  *
  * Octree nodes store application data, a list of pointers to child nodes (or NULL) and
  * a pointer to their direct octree node parent.
  */
template< typename T_, int d_ = 3, typename A_ = std::allocator<T_> >
struct octree_node
{
  typedef octree<T_,d_,A_>* octree_pointer;
  typedef octree_node<T_,d_,A_>* octree_node_pointer;
  typedef const octree_node<T_,d_,A_>* const_octree_node_pointer;
  typedef octree_node<T_,d_,A_>& octree_node_reference;
  typedef const octree_node<T_,d_,A_>& const_octree_node_reference;
  typedef T_ value_type;
  typedef T_* pointer;
  typedef T_& reference;
  typedef const T_* const_pointer;
  typedef const T_& const_reference;

  octree_node_pointer _M_parent;
  octree_node_pointer _M_children;
  value_type _M_data;

  octree_node();
  octree_node( octree_node_pointer parent, const value_type& data );
  ~octree_node();

  bool is_leaf_node() { return this->_M_children == 0; }
  int num_children() { return this->_M_children ? (1<<d_) : 0; }
  bool add_children();
  bool add_children( const T_& child_initializer );
  bool remove_children();

  reference value() { return this->_M_data; }
  reference value() const { return this->_M_data; }

  const_octree_node_reference operator [] ( int child ) const;
  octree_node_reference operator [] ( int child );

  reference operator * () { return _M_data; }
  const_reference operator * () const { return _M_data; }
};

#endif // __octree_node
