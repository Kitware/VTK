// Included by octree.h

/**\typedef template<typename T_,typename R_,typename P_,typename O_,typename OP_,int d_> \
  *         typedef O_ octree_iterator<T_,R_,P_,O_,OP_,d_>::octree_type;
  * \brief Shorthand for an octree over which this class can iterate.
  */
/**\typedef template<typename T_,typename R_,typename P_,typename O_,typename OP_,int d_> \
  *         typedef OP_ octree_iterator<T_,R_,P_,O_,OP_,d_>::octree_pointer;
  * \brief Shorthand for a pointer to an octree over which this class can iterate.
  */
/**\typedef template<typename T_,typename R_,typename P_,typename O_,typename OP_,int d_> \
  *         typedef typename O_::allocator_type octree_iterator<T_,R_,P_,O_,OP_,d_>::octree_allocator_type;
  * \brief Shorthand for the allocator used by the octrees over which this class iterates.
  */
/**\typedef template<typename T_,typename R_,typename P_,typename O_,typename OP_,int d_> \
  *         typedef typename O_::octree_node_reference octree_iterator<T_,R_,P_,O_,OP_,d_>::octree_node_reference;
  * \brief Shorthand for a reference to a node in the octree.
  */
/**\typedef template<typename T_,typename R_,typename P_,typename O_,typename OP_,int d_> \
  *         typedef typename O_::octree_node_pointer octree_iterator<T_,R_,P_,O_,OP_,d_>::octree_node_pointer;
  * \brief Shorthand for a pointer to a node in the octree.
  */
/**\typedef template<typename T_,typename R_,typename P_,typename O_,typename OP_,int d_> \
  *         typedef std::bidirectional_iterator_tag octree_iterator<T_,R_,P_,O_,OP_,d_>::iterator_category;
  * \brief A tag used by the STL to determine what algorithms may be applied to the octree container.
  */
/**\typedef template<typename T_,typename R_,typename P_,typename O_,typename OP_,int d_> \
  *         typedef octree_iterator< T_, T_&, T_*, O_, O_*, d_ > octree_iterator<T_,R_,P_,O_,OP_,d_>::iterator;
  * \brief Shorthand for a non-const octree iterator (regardless of whether the current iterator is const or not).
  */
/**\typedef template<typename T_,typename R_,typename P_,typename O_,typename OP_,int d_> \
  *         typedef octree_iterator< T_, const T_&, const T_*, O_, const O_*, d_ > octree_iterator<T_,R_,P_,O_,OP_,d_>::const_iterator;
  * \brief Shorthand for a const octree iterator (regardless of whether the current iterator is const or not).
  */
/**\typedef template<typename T_,typename R_,typename P_,typename O_,typename OP_,int d_> \
  *         typedef octree_iterator< T_, R_, P_, O_, OP_, d_ > octree_iterator<T_,R_,P_,O_,OP_,d_>::self_iterator;
  * \brief Shorthand for an iterator of the same type as the current iterator (be it const or not).
  */

/**\var template<typename T_,typename R_,typename P_,typename O_,typename OP_,int d_> \
  *     bool octree_iterator<T_,R_,P_,O_,OP_,d_>::_M_immediate_family
  *\brief Iterate over all the subnodes or just the direct children?
  */
/**\var template<typename T_,typename R_,typename P_,typename O_,typename OP_,int d_> \
  *     bool octree_iterator<T_,R_,P_,O_,OP_,d_>::_M_only_leaf_nodes
  *\brief Should the iterator visit all nodes or only leaf nodes?
  */

/**\brief Default constructor.
  */
template< typename T_, typename R_, typename P_, typename O_, typename OP_, int d_ >
octree_iterator<T_,R_,P_,O_,OP_,d_>::octree_iterator()
{
  this->_M_only_leaf_nodes = true;
  this->_M_immediate_family = false;
}

/**\brief Constructor for use by octree_type's begin() and end() methods.
  *
  */
template< typename T_, typename R_, typename P_, typename O_, typename OP_, int d_ >
octree_iterator<T_,R_,P_,O_,OP_,d_>::octree_iterator( octree_node_pointer oroot, octree_node_pointer onode, bool only_leaves )
  : octree_path<T_,R_,P_,O_,OP_,d_>( oroot )
{
  // WARNING: This method assumes onode is either the root node of the tree or the "tail" of the sequence.
  // In order to be completely general, this method would have to search the entire octree for onode and
  // maintain _M_indices & _M_current_index as it searched.
  this->_M_only_leaf_nodes = only_leaves;
  this->_M_immediate_family = false;
  this->_M_current_node = onode;
  if ( this->_M_only_leaf_nodes )
    {
    while ( this->_M_current_node && this->_M_current_node->_M_children )
      {
      this->_M_indices.push_back( 0 );
      this->_M_parents.push_back( this->_M_current_node );
      this->_M_current_node = this->_M_current_node->_M_children;
      }
    }
}

/**\brief A copy constructor.
  *
  */
template< typename T_, typename R_, typename P_, typename O_, typename OP_, int d_ >
octree_iterator<T_,R_,P_,O_,OP_,d_>::octree_iterator( const const_iterator& it )
{
  this->_M_root = it._M_root;
  this->_M_immediate_family = it._M_immediate_family;
  this->_M_only_leaf_nodes = it._M_only_leaf_nodes;
  this->_M_indices = it._M_indices;
  this->_M_parents = it._M_parents;
  this->_M_current_node = it._M_current_node;
}

/**\brief Destructor.
  *
  */
template< typename T_, typename R_, typename P_, typename O_, typename OP_, int d_ >
octree_iterator<T_,R_,P_,O_,OP_,d_>::~octree_iterator()
{
}

/**\brief Utility routine used to advance the iterator if possible.
  *
  */
template< typename T_, typename R_, typename P_, typename O_, typename OP_, int d_ >
typename octree_iterator<T_,R_,P_,O_,OP_,d_>::octree_node_pointer octree_iterator<T_,R_,P_,O_,OP_,d_>::check_incr()
{
  if ( ! this->_M_root )
    {
    throw std::logic_error( "Can't increment iterator with null octree pointer." );
    }
  // Oldtown. (We're at the end)
  if ( ! this->_M_current_node )
    {
    return 0;
    }
  int child = 0;
  // Uptown. (Climb upwards to the first non-traversed, non-leaf node)
  if ( this->_M_immediate_family )
    {
    if ( this->_M_indices.empty() )
      {
      return 0;
      }
      this->_M_current_node = this->_M_parents.back();
      child = this->_M_indices.back() + 1;
      this->_M_parents.pop_back();
      this->_M_indices.pop_back();
      if ( child >= (1<<d_) )
        {
        this->_M_current_node = 0; // move to the end, but don't clear out parents/indices
        return 0;
        }
    }
  else if ( this->_M_current_node->is_leaf_node() )
    {
    while ( 1 )
      {
      if ( this->_M_indices.empty() )
        {
        return 0;
        }
      this->_M_current_node = this->_M_parents.back();
      child = this->_M_indices.back() + 1;
      this->_M_parents.pop_back();
      this->_M_indices.pop_back();
      if ( child < (1<<d_) )
        {
        break;
        }
      }
    }
  // Downtown. (Climb down to the next node)
  while ( ! this->_M_current_node->is_leaf_node() )
    {
    this->_M_parents.push_back( this->_M_current_node );
    this->_M_indices.push_back( child );
    this->_M_current_node = this->_M_current_node->_M_children + child;
    child = 0;
    if ( ! this->_M_only_leaf_nodes || this->_M_immediate_family )
      break;
    }
  return this->_M_current_node;
}

/**\brief Utility routine used to back up the iterator if possible.
  *
  */
template< typename T_, typename R_, typename P_, typename O_, typename OP_, int d_ >
typename octree_iterator<T_,R_,P_,O_,OP_,d_>::octree_node_pointer octree_iterator<T_,R_,P_,O_,OP_,d_>::check_decr()
{
  if ( ! this->_M_root )
    {
    throw std::logic_error( "Can't decrement iterator with null octree pointer." );
    }
  // Oldtown. (We're at the end)
  if ( ! this->_M_current_node )
    { // Descend to last leaf
    this->_M_current_node = this->_M_root;
    while ( this->_M_current_node && this->_M_current_node->_M_children )
      {
      int child = (1<<d_) - 1;
      this->_M_indices.push_back( child );
      this->_M_parents.push_back( this->_M_current_node );
      this->_M_current_node = this->_M_current_node->_M_children + child;
      }
    return this->_M_current_node;
    }
  else if ( this->_M_current_node == this->_M_root )
    { // We're at the beginning node... can't go back any more
    // FIXME: What do we do when traversing only leaf nodes? The root isn't the beginning any more...
    octree_node_pointer tmp = this->_M_current_node;
    //this->_M_current_node = 0; // go back to "end" after reporting the beginning node.
    return tmp;
    }
  int child = (1<<d_) - 1;
  // Uptown. (Climb upwards to the first non-traversed node)
  while ( 1 )
    {
    if ( this->_M_indices.empty() )
      { // Last node is root node. Report it if we should.
      if (
        ( this->_M_only_leaf_nodes && this->_M_current_node->is_leaf_node() ) ||
        ( ! this->_M_only_leaf_nodes ) )
        {
        return this->_M_current_node;
        }
      else
        { // Oops, we were already at "begin()"... go past end.
        octree_node_pointer tmp = this->_M_current_node;
        //this->_M_current_node = 0; // go back to "end" after reporting the beginning node.
        return tmp;
        }
      }
    this->_M_current_node = this->_M_parents.back();
    child = this->_M_indices.back() - 1;
    this->_M_parents.pop_back();
    this->_M_indices.pop_back();
    if ( this->_M_only_leaf_nodes )
      {
      if ( child >= 0 )
        {
        break;
        }
      }
    else
      {
      if ( child >= -1 )
        {
        break;
        }
      }
    }
  // Midtown. (Stop at non-leaf nodes if so ordered)
  if ( child < 0 )
    {
    return this->_M_current_node;
    }
  // Downtown. (Climb down to the next node)
  while ( ! this->_M_current_node->is_leaf_node() )
    {
    this->_M_parents.push_back( this->_M_current_node );
    this->_M_indices.push_back( child );
    this->_M_current_node = this->_M_current_node->_M_children + child;
    child = (1<<d_) - 1;
    }
  return this->_M_current_node;
}

/**\brief Force the iterator to traverse only siblings and not children or parent nodes.
  *
  */
template< typename T_, typename R_, typename P_, typename O_, typename OP_, int d_ >
void octree_iterator<T_,R_,P_,O_,OP_,d_>::immediate_family( bool val )
{
  this->_M_immediate_family = val;
}

/**\fn template< typename T_, typename R_, typename P_, typename O_, typename OP_, int d_ > \
  *    bool octree_iterator<T_,R_,P_,O_,OP_,d_>::immediate_family() const
  *\brief Return whether the iterator is set to traverse the entire tree or just the siblings of the current node.
  */

/**\fn template< typename T_, typename R_, typename P_, typename O_, typename OP_, int d_ > \
  *    bool& octree_iterator<T_,R_,P_,O_,OP_,d_>::immediate_family()
  *\brief Return whether the iterator is set to traverse the entire tree or just the siblings of the current node.
  */

/**\brief Assignment operator (for copying iterators of mutable nodes).
  *
  */
template< typename T_, typename R_, typename P_, typename O_, typename OP_, int d_ >
octree_iterator< T_, R_, P_, O_, OP_, d_ >& octree_iterator<T_,R_,P_,O_,OP_,d_>::operator = ( const iterator& it )
{
  this->octree_path<T_,R_,P_,O_,OP_,d_>::operator=( it );
  this->_M_immediate_family = it._M_immediate_family;
  this->_M_only_leaf_nodes = it._M_only_leaf_nodes;
  return *this;
}

/**\brief Assignment operator (for copying iterators of immutable nodes).
  *
  */
#if ! ( defined(_MSC_VER) && (_MSC_VER < 1300) )
template< typename T_, typename R_, typename P_, typename O_, typename OP_, int d_ >
octree_iterator< T_, R_, P_, O_, OP_, d_ >& octree_iterator<T_,R_,P_,O_,OP_,d_>::operator = ( const const_iterator& it )
{
  this->octree_path<T_,R_,P_,O_,OP_,d_>::operator=( it );
  this->_M_immediate_family = it._M_immediate_family;
  this->_M_only_leaf_nodes = it._M_only_leaf_nodes;
  return *this;
}
#endif

/**\fn template< typename T_, typename R_, typename P_, typename O_, typename OP_, int d_ > \
  *    self_iterator& octree_iterator<T_,R_,P_,O_,OP_,d_>::operator ++ ()
  *\brief Move to the next node in the octree that satisfies the traversal criteria.
  */

/**\fn template< typename T_, typename R_, typename P_, typename O_, typename OP_, int d_ > \
  *    self_iterator  octree_iterator<T_,R_,P_,O_,OP_,d_>::operator ++ ( int )
  *\brief Move to the next node in the octree that satisfies the traversal criteria.
  */

/**\fn template< typename T_, typename R_, typename P_, typename O_, typename OP_, int d_ > \
  *    self_iterator& octree_iterator<T_,R_,P_,O_,OP_,d_>::operator -- ()
  *\brief Move to the previous node in the octree that satisfied the traversal criteria.
  */

/**\fn template< typename T_, typename R_, typename P_, typename O_, typename OP_, int d_ > \
  *    self_iterator  octree_iterator<T_,R_,P_,O_,OP_,d_>::operator -- ( int )
  *\brief Move to the previous node in the octree that satisfied the traversal criteria.
  */

