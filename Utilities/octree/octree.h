#ifndef __octree_h
#define __octree_h

#include <iostream>

/**\brief An n-dimensional octree. Perhaps it should be named a tttntree (two-to-the n tree)?
  *
  */
template< typename T_, int d_ = 3, typename A_ = vtkstd::allocator<T_> >
class octree
{
public:
  typedef T_ value_type;
  typedef T_* pointer;
  typedef T_& reference;

  typedef const T_* const_pointer;
  typedef const T_& const_reference;

  typedef octree<T_,d_,A_> _self_type;
  typedef _self_type* _self_pointer;

  typedef octree_node<T_,d_,A_>* octree_node_pointer;
  typedef octree_node<T_,d_,A_>& octree_node_reference;
  typedef const octree_node<T_,d_,A_>* const_octree_node_pointer;
  typedef const octree_node<T_,d_,A_>& const_octree_node_reference;

  typedef typename vtkstd::vector<octree_node_pointer>::size_type size_type;

  typedef A_ allocator_type;

  // Ugly. But neccessary according to young me. Old me says so.
  typedef octree_iterator< T_, T_&, T_*, _self_type, _self_pointer, d_ > iterator;
  typedef octree_iterator< T_, const T_&, const T_*, _self_type, _self_pointer, d_ > const_iterator;

  typedef octree_cursor< T_, T_&, T_*, _self_type, _self_pointer, d_ > cursor;
  typedef octree_cursor< T_, const T_&, const T_*, _self_type, _self_pointer, d_ > const_cursor;

  octree( const double* center, double size );
  octree( const double* center, double size, const value_type& root_node_value );
  virtual ~octree();

  /** \brief Iterator based access.
    *
    * Iterators come in const and non-const versions as well as versions
    * that visit all nodes or just leaf nodes. By default, only leaf nodes
    * are visited.
    *
    * You may add or remove children while iterating with some caveats.
    * When adding children, any iterator currently referencing the node
    * to which children were added will reference the first child node
    * when incremented. Iterators confined to leaf nodes may reference
    * non-leaf nodes when refinement occurs between increments/decrements.
    * When removing children from node \a A, any iterators pointing to
    * grandchildren of \a A will become invalid. Iterators pointing to
    * children of \a A may not be dereferenced but may be incremented or
    * decremented safely.
    */
  //@{
  iterator begin( bool only_leaves = true ) { return iterator( _M_root, _M_root, only_leaves ); }
  iterator end( bool only_leaves = true ) { return iterator( _M_root, 0, only_leaves ); }

  const_iterator begin( bool only_leaves = true ) const { return const_iterator( _M_root, _M_root, only_leaves ); }
  const_iterator end( bool only_leaves = true ) const { return const_iterator( _M_root, 0, only_leaves ); }
  //@}
  
  octree_node_pointer root() { return this->_M_root; }

  size_t size( bool only_leaves = false );

  /** \brief Geometric information
    * Binary trees of dimension 2 or higher are often used to partition geometric objects into subsets.
    * In order to do this, the tree must have some geometric center and size.
    * These are set by the constructor but may be queried at any time.
    * They may not be modified as that would require a re-partitioning of the objects (typically stored
    * at nodes or leaf-nodes).
    */
  //@{
  const double* center() const { return this->_M_center; }
  double size() const { return this->_M_size; }
  //@}

protected:
  octree_node_pointer _M_root;
  double _M_center[d_];
  double _M_size;
};

#endif // __octree_h

