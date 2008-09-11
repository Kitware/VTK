// Included by octree.h

/**\typedef template<typename _T,typename _R,typename _P,typename _O,typename _OP,int _d> \
  *         typedef _O octree_cursor<_T,_R,_P,_O,_OP,_d>::octree_type;
  * \brief Shorthand for an octree over which this class can iterate.
  */
/**\typedef template<typename _T,typename _R,typename _P,typename _O,typename _OP,int _d> \
  *         typedef _OP octree_cursor<_T,_R,_P,_O,_OP,_d>::octree_pointer;
  * \brief Shorthand for a pointer to an octree over which this class can iterate.
  */
/**\typedef template<typename _T,typename _R,typename _P,typename _O,typename _OP,int _d> \
  *         typedef typename _O::allocator_type octree_cursor<_T,_R,_P,_O,_OP,_d>::octree_allocator_type;
  * \brief Shorthand for the allocator used by the octrees over which this class iterates.
  */
/**\typedef template<typename _T,typename _R,typename _P,typename _O,typename _OP,int _d> \
  *         typedef typename _O::octree_node_reference octree_cursor<_T,_R,_P,_O,_OP,_d>::octree_node_reference;
  * \brief Shorthand for a reference to a node in the octree.
  */
/**\typedef template<typename _T,typename _R,typename _P,typename _O,typename _OP,int _d> \
  *         typedef typename _O::octree_node_pointer octree_cursor<_T,_R,_P,_O,_OP,_d>::octree_node_pointer;
  * \brief Shorthand for a pointer to a node in the octree.
  */

/**\typedef template<typename _T,typename _R,typename _P,typename _O,typename _OP,int _d> \
  *         typedef octree_cursor< _T, _T&, _T*, _O, _O*, _d > octree_cursor<_T,_R,_P,_O,_OP,_d>::path;
  * \brief Shorthand for a non-const octree path (regardless of whether the current path is const or not).
  */
/**\typedef template<typename _T,typename _R,typename _P,typename _O,typename _OP,int _d> \
  *         typedef octree_cursor< _T, const _T&, const _T*, _O, const _O*, _d > octree_cursor<_T,_R,_P,_O,_OP,_d>::const_path;
  * \brief Shorthand for a const octree path (regardless of whether the current path is const or not).
  */
/**\typedef template<typename _T,typename _R,typename _P,typename _O,typename _OP,int _d> \
  *         typedef octree_cursor< _T, _R, _P, _O, _OP, _d > octree_cursor<_T,_R,_P,_O,_OP,_d>::self_path;
  * \brief Shorthand for a path of the same type as the current path (be it const or not).
  */

/**\typedef template<typename _T,typename _R,typename _P,typename _O,typename _OP,int _d> \
  *         typedef octree_cursor< _T, _T&, _T*, _O, _O*, _d > octree_cursor<_T,_R,_P,_O,_OP,_d>::cursor;
  * \brief Shorthand for a non-const octree cursor (regardless of whether the current cursor is const or not).
  */
/**\typedef template<typename _T,typename _R,typename _P,typename _O,typename _OP,int _d> \
  *         typedef octree_cursor< _T, const _T&, const _T*, _O, const _O*, _d > octree_cursor<_T,_R,_P,_O,_OP,_d>::const_cursor;
  * \brief Shorthand for a const octree cursor (regardless of whether the current cursor is const or not).
  */
/**\typedef template<typename _T,typename _R,typename _P,typename _O,typename _OP,int _d> \
  *         typedef octree_cursor< _T, _R, _P, _O, _OP, _d > octree_cursor<_T,_R,_P,_O,_OP,_d>::self_cursor;
  * \brief Shorthand for an cursor of the same type as the current cursor (be it const or not).
  */

/**\brief Default constructor. Not very useful since there's no way to indicate the octree.
  */
template< typename _T, typename _R, typename _P, typename _O, typename _OP, int _d >
octree_cursor<_T,_R,_P,_O,_OP,_d>::octree_cursor()
{
}

/**\brief Constructor you should generally use.
  *
  */
template< typename _T, typename _R, typename _P, typename _O, typename _OP, int _d >
octree_cursor<_T,_R,_P,_O,_OP,_d>::octree_cursor( octree_pointer otree )
  : octree_path<_T,_R,_P,_O,_OP,_d>( otree )
{
}

/**\brief A copy constructor.
  *
  * Note that this constructor can copy anything derived from octree_path, not just other octree_cursor objects.
  * In particular, this means you can create a cursor from an iterator and then move around the octree using
  * cursor operations. The inverse (creating an iterator from a cursor) is not provided; there should be little
  * need for it and it is unclear what to do in certain situations (e.g., when a cursor points to an octree node
  * that would not normally be iterated).
  */
template< typename _T, typename _R, typename _P, typename _O, typename _OP, int _d >
octree_cursor<_T,_R,_P,_O,_OP,_d>::octree_cursor( const const_path& src )
{
  this->_M_octree = src._M_octree;
  this->_M_indices = src._M_indices;
  this->_M_parents = src._M_parents;
  this->_M_current_node = src._M_current_node;
}

/**\brief Destructor.
  *
  */
template< typename _T, typename _R, typename _P, typename _O, typename _OP, int _d >
octree_cursor<_T,_R,_P,_O,_OP,_d>::~octree_cursor()
{
}

/**\brief Move the cursor up one level.
  *
  * If this is called when the cursor is on the root node, it has no effect.
  */
template< typename _T, typename _R, typename _P, typename _O, typename _OP, int _d >
void octree_cursor<_T,_R,_P,_O,_OP,_d>::up()
{
  if ( this->_M_indices.size() )
    {
    this->_M_current_node = this->_M_parents.back();
    this->_M_indices.pop_back();
    this->_M_parents.pop_back();
    }
}

/**\brief Move the cursor down to the specified child.
  *
  * If this is called when the cursor is at a leaf node, it has no effect.
  */
template< typename _T, typename _R, typename _P, typename _O, typename _OP, int _d >
void octree_cursor<_T,_R,_P,_O,_OP,_d>::down( int child_of_this_node )
{
  if ( this->_M_current_node->is_leaf_node() )
    {
    return;
    }
  if ( child_of_this_node < 0 || child_of_this_node > (1<<_d) )
    {
    throw vtkstd::range_error( "Invalid child node specified." );
    }
  this->_M_parents.push_back( this->_M_current_node );
  this->_M_indices.push_back( child_of_this_node );
  this->_M_current_node = &( (*this->_M_current_node)[child_of_this_node] );
}

/**\brief Return where in the current level() the cursor is located.
  *
  * Returns the index into the children of the current node's parent where the current node is located.
  * A -1 is returned at level 0 (i.e., when the cursor is at the root node of the tree).
  *
  * @retval An integer in \f$\left\{-1,0,\ldots,2^{\mathrm{\texttt{\_d}}}-1\right\}\f$.
  */
template< typename _T, typename _R, typename _P, typename _O, typename _OP, int _d >
int octree_cursor<_T,_R,_P,_O,_OP,_d>::where() const
{
  if ( this->_M_indices.size() <= 0 )
    {
    return -1;
    }
  return this->_M_indices.back();
}

/**\brief Move to a different child with the same parent.
  *
  * This function has no effect when the cursor is currently on the root node.
  * This can throw vtkstd::range_error when \a child_of_shared_parent is invalid.
  *
  * @param[in] child_of_shared_parent the child of the parent node to which the cursor should move.
  *            This is an integer in \f$\left\{0,\ldots,2^{\mathrm{\texttt{\_d}}}-1\right\}\f$.
  */
template< typename _T, typename _R, typename _P, typename _O, typename _OP, int _d >
void octree_cursor<_T,_R,_P,_O,_OP,_d>::over( int child_of_shared_parent )
{
  if ( this->_M_indices.size() <= 0 )
    {
    return;
    }

  if ( child_of_shared_parent < 0 || child_of_shared_parent >= (1<<_d) )
    {
    throw vtkstd::range_error( "Invalid sibling specified." );
    }

  this->_M_indices.back() = child_of_shared_parent;
  this->_M_current_node = &( (*this->_M_parents.back())[child_of_shared_parent] );
}

/**\brief Move to the other sibling node along a given \a axis.
  *
  * Move the cursor to the sibling which occupies the other quadrant/octant/... along
  * the given \a axis while sharing the same bounds on all other axes.
  * This will throw a vtkstd::logic_error when the cursor is at the root of the tree.
  * It will throw a vtkstd::range_error when the axis is invalid.
  *
  * @param axis An integer in \f$\left\{0,\ldots,\mathrm{\texttt{\_d}}-1\right\}\f$
  */
template< typename _T, typename _R, typename _P, typename _O, typename _OP, int _d >
void octree_cursor<_T,_R,_P,_O,_OP,_d>::axis_partner( int axis )
{
  if ( axis < 0 || axis >= (1<<_d) )
    {
    throw vtkstd::range_error( "An invalid axis was specified." );
    }

  int bitcode = this->where();
  if ( bitcode < 0 )
    {
    throw vtkstd::logic_error( "The root node has no axis partner." );
    }

  bitcode = (bitcode & ~(1<<axis)) | (bitcode ^ (1<<axis));
  this->_M_indices.back() = bitcode;
  this->_M_current_node = &( (*this->_M_parents.back())[bitcode] );
}

/**\brief Determine whether the cursor is pointing to a lower or upper quadrant/octant/... of its parent along the given axis.
  *
  * This will throw a vtkstd::logic_error when the cursor is at the root of the tree.
  * It will throw a vtkstd::range_error when the axis is invalid.
  *
  * @param[in] axis The axis of interest. An integer in \f$\left\{0,\ldots,\mathrm{\texttt{\_d}}-1\right\}\f$.
  * @retval         0 if the cursor points to a lower quadrant/octant, 1 if the cursor points to an upper quadrant/octant.
  */
template< typename _T, typename _R, typename _P, typename _O, typename _OP, int _d >
bool octree_cursor<_T,_R,_P,_O,_OP,_d>::axis_bit( int axis ) const
{
  if ( axis < 0 || axis >= (1<<_d) )
    {
    throw vtkstd::range_error( "An invalid axis was specified." );
    }

  int bitcode = this->where();
  if ( bitcode < 0 )
    {
    throw vtkstd::logic_error( "The root node has no axis partner." );
    }

  return ( bitcode & (1<<axis) ) ? 1 : 0;
}

/**\brief Visit a specified octree node if it exists.
  *
  * Given a vector of integers specifying, for each level of the octree to descend, which child
  * node should be chosen, return true if the specified path exists and false otherwise.
  * If the path exists, the cursor will point to the specified node upon return.
  * Otherwise, the cursor will remain unchanged.
  */
template< typename _T, typename _R, typename _P, typename _O, typename _OP, int _d >
bool octree_cursor<_T,_R,_P,_O,_OP,_d>::visit( const vtkstd::vector<int>& pathSpec )
{
  vtkstd::vector<int>::const_iterator it;
  vtkstd::vector<octree_node_pointer> parents;
  octree_node_pointer head = this->_M_octree->root();
  for ( it = pathSpec.begin(); it != pathSpec.end(); ++ it )
    {
    parents.push_back( head );
    if ( ( *it < 0 ) || ( *it >= head->num_children() ) )
      { // Oops, missing a node.
      return false;
      }
    head = head->_M_children + *it;
    }
  // Made it all the way through the path as specified.
  this->_M_parents = parents;
  this->_M_indices = pathSpec;
  this->_M_current_node = head;
  return true;
}

/**\brief Assignment operator (for copying paths of mutable nodes).
  *
  */
template< typename _T, typename _R, typename _P, typename _O, typename _OP, int _d >
octree_path<_T,_R,_P,_O,_OP,_d>& octree_cursor<_T,_R,_P,_O,_OP,_d>::operator = ( const path& it )
{
  return this->octree_path<_T,_R,_P,_O,_OP,_d>::operator=( it );
}

/**\brief Assignment operator (for copying paths of immutable nodes).
  *
  */
template< typename _T, typename _R, typename _P, typename _O, typename _OP, int _d >
octree_path<_T,_R,_P,_O,_OP,_d>& octree_cursor<_T,_R,_P,_O,_OP,_d>::operator = ( const const_path& it )
{
  return this->octree_path<_T,_R,_P,_O,_OP,_d>::operator=( it );
}

