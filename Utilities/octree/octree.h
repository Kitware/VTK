#ifndef __octree_h
#define __octree_h

#include <iostream>

/**\brief An n-dimensional octree. Perhaps it should be named a tttntree (two-to-the n tree)?
  *
  */
template< typename _T, int _d = 3, typename _A = vtkstd::allocator<_T> >
class octree
{
public:
  typedef _T value_type;
  typedef _T* pointer;
  typedef _T& reference;

  typedef const _T* const_pointer;
  typedef const _T& const_reference;

  typedef octree<_T,_d,_A> _self_type;
  typedef _self_type* _self_pointer;

  typedef octree_node<_T,_d,_A>* octree_node_pointer;
  typedef octree_node<_T,_d,_A>& octree_node_reference;
  typedef const octree_node<_T,_d,_A>* const_octree_node_pointer;
  typedef const octree_node<_T,_d,_A>& const_octree_node_reference;

  typedef _A allocator_type;

  // Ugly. But neccessary according to young me. Old me says so.
  typedef octree_iterator< _T, _T&, _T*, _self_type, _self_pointer, _d > iterator;
  typedef octree_iterator< _T, const _T&, const _T*, _self_type, _self_pointer, _d > const_iterator;

  typedef octree_cursor< _T, _T&, _T*, _self_type, _self_pointer, _d > cursor;
  typedef octree_cursor< _T, const _T&, const _T*, _self_type, _self_pointer, _d > const_cursor;

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
  iterator begin( bool only_leaves = true ) { return iterator( this, _M_root, only_leaves ); }
  iterator end( bool only_leaves = true ) { return iterator( this, 0, only_leaves ); }

  const_iterator begin( bool only_leaves = true ) const { return const_iterator( this, _M_root, only_leaves ); }
  const_iterator end( bool only_leaves = true ) const { return const_iterator( this, 0, only_leaves ); }
  //@}
  
  octree_node_pointer root() { return this->_M_root; }

  size_t size( bool only_leaves = false );

protected:
  octree_node_pointer _M_root;
};

#endif // __octree_h

