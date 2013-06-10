// Included by octree.h

/**\typedef template<typename T_,typename R_,typename P_,typename O_,typename OP_,int d_> \
  *         typedef O_ octree_cursor<T_,R_,P_,O_,OP_,d_>::octree_type;
  * \brief Shorthand for an octree over which this class can iterate.
  */
/**\typedef template<typename T_,typename R_,typename P_,typename O_,typename OP_,int d_> \
  *         typedef OP_ octree_cursor<T_,R_,P_,O_,OP_,d_>::octree_pointer;
  * \brief Shorthand for a pointer to an octree over which this class can iterate.
  */
/**\typedef template<typename T_,typename R_,typename P_,typename O_,typename OP_,int d_> \
  *         typedef typename O_::allocator_type octree_cursor<T_,R_,P_,O_,OP_,d_>::octree_allocator_type;
  * \brief Shorthand for the allocator used by the octrees over which this class iterates.
  */
/**\typedef template<typename T_,typename R_,typename P_,typename O_,typename OP_,int d_> \
  *         typedef typename O_::octree_node_reference octree_cursor<T_,R_,P_,O_,OP_,d_>::octree_node_reference;
  * \brief Shorthand for a reference to a node in the octree.
  */
/**\typedef template<typename T_,typename R_,typename P_,typename O_,typename OP_,int d_> \
  *         typedef typename O_::octree_node_pointer octree_cursor<T_,R_,P_,O_,OP_,d_>::octree_node_pointer;
  * \brief Shorthand for a pointer to a node in the octree.
  */

/**\typedef template<typename T_,typename R_,typename P_,typename O_,typename OP_,int d_> \
  *         typedef octree_cursor< T_, T_&, T_*, O_, O_*, d_ > octree_cursor<T_,R_,P_,O_,OP_,d_>::path;
  * \brief Shorthand for a non-const octree path (regardless of whether the current path is const or not).
  */
/**\typedef template<typename T_,typename R_,typename P_,typename O_,typename OP_,int d_> \
  *         typedef octree_cursor< T_, const T_&, const T_*, O_, const O_*, d_ > octree_cursor<T_,R_,P_,O_,OP_,d_>::const_path;
  * \brief Shorthand for a const octree path (regardless of whether the current path is const or not).
  */
/**\typedef template<typename T_,typename R_,typename P_,typename O_,typename OP_,int d_> \
  *         typedef octree_cursor< T_, R_, P_, O_, OP_, d_ > octree_cursor<T_,R_,P_,O_,OP_,d_>::self_path;
  * \brief Shorthand for a path of the same type as the current path (be it const or not).
  */

/**\typedef template<typename T_,typename R_,typename P_,typename O_,typename OP_,int d_> \
  *         typedef octree_cursor< T_, T_&, T_*, O_, O_*, d_ > octree_cursor<T_,R_,P_,O_,OP_,d_>::cursor;
  * \brief Shorthand for a non-const octree cursor (regardless of whether the current cursor is const or not).
  */
/**\typedef template<typename T_,typename R_,typename P_,typename O_,typename OP_,int d_> \
  *         typedef octree_cursor< T_, const T_&, const T_*, O_, const O_*, d_ > octree_cursor<T_,R_,P_,O_,OP_,d_>::const_cursor;
  * \brief Shorthand for a const octree cursor (regardless of whether the current cursor is const or not).
  */
/**\typedef template<typename T_,typename R_,typename P_,typename O_,typename OP_,int d_> \
  *         typedef octree_cursor< T_, R_, P_, O_, OP_, d_ > octree_cursor<T_,R_,P_,O_,OP_,d_>::self_cursor;
  * \brief Shorthand for an cursor of the same type as the current cursor (be it const or not).
  */

/**\brief Default constructor. Not very useful since there's no way to indicate the octree.
  */
template< typename T_, typename R_, typename P_, typename O_, typename OP_, int d_ >
octree_cursor<T_,R_,P_,O_,OP_,d_>::octree_cursor()
{
}

/**\brief Constructor you should generally use.
  *
  */
template< typename T_, typename R_, typename P_, typename O_, typename OP_, int d_ >
octree_cursor<T_,R_,P_,O_,OP_,d_>::octree_cursor( octree_pointer otree )
  : octree_path<T_,R_,P_,O_,OP_,d_>( otree->root() )
{
}

/**\brief Constructor you should generally use.
  *
  */
template< typename T_, typename R_, typename P_, typename O_, typename OP_, int d_ >
octree_cursor<T_,R_,P_,O_,OP_,d_>::octree_cursor( octree_node_pointer oroot )
  : octree_path<T_,R_,P_,O_,OP_,d_>( oroot )
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
template< typename T_, typename R_, typename P_, typename O_, typename OP_, int d_ >
octree_cursor<T_,R_,P_,O_,OP_,d_>::octree_cursor( const const_path& src )
{
  this->_M_root = src._M_root;
  this->_M_indices = src._M_indices;
  this->_M_parents = src._M_parents;
  this->_M_current_node = src._M_current_node;
}

/**\brief Destructor.
  *
  */
template< typename T_, typename R_, typename P_, typename O_, typename OP_, int d_ >
octree_cursor<T_,R_,P_,O_,OP_,d_>::~octree_cursor()
{
}

/**\brief Move the cursor up one level.
  *
  * If this is called when the cursor is on the root node, it has no effect.
  */
template< typename T_, typename R_, typename P_, typename O_, typename OP_, int d_ >
void octree_cursor<T_,R_,P_,O_,OP_,d_>::up()
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
template< typename T_, typename R_, typename P_, typename O_, typename OP_, int d_ >
void octree_cursor<T_,R_,P_,O_,OP_,d_>::down( int child_of_this_node )
{
  if ( this->_M_current_node->is_leaf_node() )
    {
    return;
    }
  if ( child_of_this_node < 0 || child_of_this_node > (1<<d_) )
    {
    throw std::range_error( "Invalid child node specified." );
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
  * @retval An integer in \f$\left\{-1,0,\ldots,2^{\mathrm{\texttt{d\_}}}-1\right\}\f$.
  */
template< typename T_, typename R_, typename P_, typename O_, typename OP_, int d_ >
int octree_cursor<T_,R_,P_,O_,OP_,d_>::where() const
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
  * This can throw std::range_error when \a child_of_shared_parent is invalid.
  *
  * @param[in] child_of_shared_parent the child of the parent node to which the cursor should move.
  *            This is an integer in \f$\left\{0,\ldots,2^{\mathrm{\texttt{d\_}}}-1\right\}\f$.
  */
template< typename T_, typename R_, typename P_, typename O_, typename OP_, int d_ >
void octree_cursor<T_,R_,P_,O_,OP_,d_>::over( int child_of_shared_parent )
{
  if ( this->_M_indices.size() <= 0 )
    {
    return;
    }

  if ( child_of_shared_parent < 0 || child_of_shared_parent >= (1<<d_) )
    {
    throw std::range_error( "Invalid sibling specified." );
    }

  this->_M_indices.back() = child_of_shared_parent;
  this->_M_current_node = &( (*this->_M_parents.back())[child_of_shared_parent] );
}

/**\brief Move to the other sibling node along a given \a axis.
  *
  * Move the cursor to the sibling which occupies the other quadrant/octant/... along
  * the given \a axis while sharing the same bounds on all other axes.
  * This will throw a std::logic_error when the cursor is at the root of the tree.
  * It will throw a std::range_error when the axis is invalid.
  *
  * @param axis An integer in \f$\left\{0,\ldots,\mathrm{\texttt{d\_}}-1\right\}\f$
  */
template< typename T_, typename R_, typename P_, typename O_, typename OP_, int d_ >
void octree_cursor<T_,R_,P_,O_,OP_,d_>::axis_partner( int axis )
{
  if ( axis < 0 || axis >= d_ )
    {
    throw std::range_error( "An invalid axis was specified." );
    }

  int bitcode = this->where();
  if ( bitcode < 0 )
    {
    throw std::logic_error( "The root node has no axis partner." );
    }

  bitcode = (bitcode & ~(1<<axis)) | (bitcode ^ (1<<axis));
  this->_M_indices.back() = bitcode;
  this->_M_current_node = &( (*this->_M_parents.back())[bitcode] );
}

/**\brief Determine whether the cursor is pointing to a lower or upper quadrant/octant/... of its parent along the given axis.
  *
  * This will throw a std::logic_error when the cursor is at the root of the tree.
  * It will throw a std::range_error when the axis is invalid.
  *
  * @param[in] axis The axis of interest. An integer in \f$\left\{0,\ldots,\mathrm{\texttt{d\_}}-1\right\}\f$.
  * @retval         0 if the cursor points to a lower quadrant/octant, 1 if the cursor points to an upper quadrant/octant.
  */
template< typename T_, typename R_, typename P_, typename O_, typename OP_, int d_ >
bool octree_cursor<T_,R_,P_,O_,OP_,d_>::axis_bit( int axis ) const
{
  if ( axis < 0 || axis >= (1<<d_) )
    {
    throw std::range_error( "An invalid axis was specified." );
    }

  int bitcode = this->where();
  if ( bitcode < 0 )
    {
    throw std::logic_error( "The root node has no axis partner." );
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
template< typename T_, typename R_, typename P_, typename O_, typename OP_, int d_ >
bool octree_cursor<T_,R_,P_,O_,OP_,d_>::visit( const std::vector<int>& pathSpec )
{
  std::vector<int>::const_iterator it;
  std::vector<octree_node_pointer> parents;
  octree_node_pointer head = this->_M_root;
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
template< typename T_, typename R_, typename P_, typename O_, typename OP_, int d_ >
octree_path<T_,R_,P_,O_,OP_,d_>& octree_cursor<T_,R_,P_,O_,OP_,d_>::operator = ( const path& it )
{
  return this->octree_path<T_,R_,P_,O_,OP_,d_>::operator=( it );
}

/**\brief Assignment operator (for copying paths of immutable nodes).
  *
  */
template< typename T_, typename R_, typename P_, typename O_, typename OP_, int d_ >
octree_path<T_,R_,P_,O_,OP_,d_>& octree_cursor<T_,R_,P_,O_,OP_,d_>::operator = ( const const_path& it )
{
  return this->octree_path<T_,R_,P_,O_,OP_,d_>::operator=( it );
}
