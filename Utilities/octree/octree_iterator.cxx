// Included by octree.h

/**\typedef template<typename _T,typename _R,typename _P,typename _O,typename _OP,int _d> \
  *         typedef _O octree_iterator<_T,_R,_P,_O,_OP,_d>::octree_type;
  * \brief Shorthand for an octree over which this class can iterate.
  */
/**\typedef template<typename _T,typename _R,typename _P,typename _O,typename _OP,int _d> \
  *         typedef _OP octree_iterator<_T,_R,_P,_O,_OP,_d>::octree_pointer;
  * \brief Shorthand for a pointer to an octree over which this class can iterate.
  */
/**\typedef template<typename _T,typename _R,typename _P,typename _O,typename _OP,int _d> \
  *         typedef typename _O::allocator_type octree_iterator<_T,_R,_P,_O,_OP,_d>::octree_allocator_type;
  * \brief Shorthand for the allocator used by the octrees over which this class iterates.
  */
/**\typedef template<typename _T,typename _R,typename _P,typename _O,typename _OP,int _d> \
  *         typedef typename _O::octree_node_reference octree_iterator<_T,_R,_P,_O,_OP,_d>::octree_node_reference;
  * \brief Shorthand for a reference to a node in the octree.
  */
/**\typedef template<typename _T,typename _R,typename _P,typename _O,typename _OP,int _d> \
  *         typedef typename _O::octree_node_pointer octree_iterator<_T,_R,_P,_O,_OP,_d>::octree_node_pointer;
  * \brief Shorthand for a pointer to a node in the octree.
  */
/**\typedef template<typename _T,typename _R,typename _P,typename _O,typename _OP,int _d> \
  *         typedef vtkstd::bidirectional_iterator_tag octree_iterator<_T,_R,_P,_O,_OP,_d>::iterator_category;
  * \brief A tag used by the STL to determine what algorithms may be applied to the octree container. 
  */
/**\typedef template<typename _T,typename _R,typename _P,typename _O,typename _OP,int _d> \
  *         typedef octree_iterator< _T, _T&, _T*, _O, _O*, _d > octree_iterator<_T,_R,_P,_O,_OP,_d>::iterator;
  * \brief Shorthand for a non-const octree iterator (regardless of whether the current iterator is const or not).
  */
/**\typedef template<typename _T,typename _R,typename _P,typename _O,typename _OP,int _d> \
  *         typedef octree_iterator< _T, const _T&, const _T*, _O, const _O*, _d > octree_iterator<_T,_R,_P,_O,_OP,_d>::const_iterator;
  * \brief Shorthand for a const octree iterator (regardless of whether the current iterator is const or not).
  */
/**\typedef template<typename _T,typename _R,typename _P,typename _O,typename _OP,int _d> \
  *         typedef octree_iterator< _T, _R, _P, _O, _OP, _d > octree_iterator<_T,_R,_P,_O,_OP,_d>::self_iterator;
  * \brief Shorthand for an iterator of the same type as the current iterator (be it const or not).
  */

/**\var template<typename _T,typename _R,typename _P,typename _O,typename _OP,int _d> \
  *     bool octree_iterator<_T,_R,_P,_O,_OP,_d>::_M_immediate_family
  *\brief Iterate over all the subnodes or just the direct children?
  */
/**\var template<typename _T,typename _R,typename _P,typename _O,typename _OP,int _d> \
  *     bool octree_iterator<_T,_R,_P,_O,_OP,_d>::_M_only_leaf_nodes
  *\brief Should the iterator visit all nodes or only leaf nodes?
  */

/**\brief Default constructor.
  */
template< typename _T, typename _R, typename _P, typename _O, typename _OP, int _d >
octree_iterator<_T,_R,_P,_O,_OP,_d>::octree_iterator()
{
  this->_M_only_leaf_nodes = true;
  this->_M_immediate_family = false;
}

/**\brief Constructor for use by octree_type's begin() and end() methods.
  *
  */
template< typename _T, typename _R, typename _P, typename _O, typename _OP, int _d >
octree_iterator<_T,_R,_P,_O,_OP,_d>::octree_iterator( octree_pointer otree, octree_node_pointer onode, bool only_leaves )
  : octree_path<_T,_R,_P,_O,_OP,_d>( otree )
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
template< typename _T, typename _R, typename _P, typename _O, typename _OP, int _d >
octree_iterator<_T,_R,_P,_O,_OP,_d>::octree_iterator( const const_iterator& it )
{
  this->_M_octree = it._M_octree;
  this->_M_immediate_family = it._M_immediate_family;
  this->_M_only_leaf_nodes = it._M_only_leaf_nodes;
  this->_M_indices = it._M_indices;
  this->_M_parents = it._M_parents;
  this->_M_current_node = it._M_current_node;
}

/**\brief Destructor.
  *
  */
template< typename _T, typename _R, typename _P, typename _O, typename _OP, int _d >
octree_iterator<_T,_R,_P,_O,_OP,_d>::~octree_iterator()
{
}

/**\brief Utility routine used to advance the iterator if possible.
  *
  */
template< typename _T, typename _R, typename _P, typename _O, typename _OP, int _d >
typename octree_iterator<_T,_R,_P,_O,_OP,_d>::octree_node_pointer octree_iterator<_T,_R,_P,_O,_OP,_d>::check_incr()
{
  if ( ! this->_M_octree )
    {
    throw vtkstd::logic_error( "Can't increment iterator with null octree pointer." );
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
      if ( child >= (1<<_d) )
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
      if ( child < (1<<_d) )
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
template< typename _T, typename _R, typename _P, typename _O, typename _OP, int _d >
typename octree_iterator<_T,_R,_P,_O,_OP,_d>::octree_node_pointer octree_iterator<_T,_R,_P,_O,_OP,_d>::check_decr()
{
  if ( ! this->_M_octree )
    {
    throw vtkstd::logic_error( "Can't decrement iterator with null octree pointer." );
    }
  // Oldtown. (We're at the end)
  if ( ! this->_M_current_node )
    { // Descend to last leaf
    this->_M_current_node = this->_M_octree->root();
    while ( this->_M_current_node && this->_M_current_node->_M_children )
      {
      int child = (1<<_d) - 1;
      this->_M_indices.push_back( child );
      this->_M_parents.push_back( this->_M_current_node );
      this->_M_current_node = this->_M_current_node->_M_children + child;
      }
    return this->_M_current_node;
    }
  else if ( this->_M_current_node == this->_M_octree->root() )
    { // We're at the beginning node... can't go back any more
    // FIXME: What do we do when traversing only leaf nodes? The root isn't the beginning any more...
    octree_node_pointer tmp = this->_M_current_node;
    //this->_M_current_node = 0; // go back to "end" after reporting the beginning node.
    return tmp;
    }
  int child = (1<<_d) - 1;
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
    child = (1<<_d) - 1;
    }
  return this->_M_current_node;
}

/**\brief Force the iterator to traverse only siblings and not children or parent nodes.
  *
  */
template< typename _T, typename _R, typename _P, typename _O, typename _OP, int _d >
void octree_iterator<_T,_R,_P,_O,_OP,_d>::immediate_family( bool val )
{
  this->_M_immediate_family = val;
}

/**\fn template< typename _T, typename _R, typename _P, typename _O, typename _OP, int _d > \
  *    bool octree_iterator<_T,_R,_P,_O,_OP,_d>::immediate_family() const
  *\brief Return whether the iterator is set to traverse the entire tree or just the siblings of the current node.
  */

/**\fn template< typename _T, typename _R, typename _P, typename _O, typename _OP, int _d > \
  *    bool& octree_iterator<_T,_R,_P,_O,_OP,_d>::immediate_family()
  *\brief Return whether the iterator is set to traverse the entire tree or just the siblings of the current node.
  */

/**\brief Assignment operator (for copying iterators of mutable nodes).
  *
  */
template< typename _T, typename _R, typename _P, typename _O, typename _OP, int _d >
octree_iterator< _T, _R, _P, _O, _OP, _d >& octree_iterator<_T,_R,_P,_O,_OP,_d>::operator = ( const iterator& it )
{
  this->octree_path<_T,_R,_P,_O,_OP,_d>::operator=( it );
  this->_M_immediate_family = it._M_immediate_family;
  this->_M_only_leaf_nodes = it._M_only_leaf_nodes;
  return *this;
}

/**\brief Assignment operator (for copying iterators of immutable nodes).
  *
  */
template< typename _T, typename _R, typename _P, typename _O, typename _OP, int _d >
octree_iterator< _T, _R, _P, _O, _OP, _d >& octree_iterator<_T,_R,_P,_O,_OP,_d>::operator = ( const const_iterator& it )
{
  this->octree_path<_T,_R,_P,_O,_OP,_d>::operator=( it );
  this->_M_immediate_family = it._M_immediate_family;
  this->_M_only_leaf_nodes = it._M_only_leaf_nodes;
  return *this;
}

/**\fn template< typename _T, typename _R, typename _P, typename _O, typename _OP, int _d > \
  *    self_iterator& octree_iterator<_T,_R,_P,_O,_OP,_d>::operator ++ ()
  *\brief Move to the next node in the octree that satisfies the traversal criteria.
  */

/**\fn template< typename _T, typename _R, typename _P, typename _O, typename _OP, int _d > \
  *    self_iterator  octree_iterator<_T,_R,_P,_O,_OP,_d>::operator ++ ( int )
  *\brief Move to the next node in the octree that satisfies the traversal criteria.
  */

/**\fn template< typename _T, typename _R, typename _P, typename _O, typename _OP, int _d > \
  *    self_iterator& octree_iterator<_T,_R,_P,_O,_OP,_d>::operator -- ()
  *\brief Move to the previous node in the octree that satisfied the traversal criteria.
  */

/**\fn template< typename _T, typename _R, typename _P, typename _O, typename _OP, int _d > \
  *    self_iterator  octree_iterator<_T,_R,_P,_O,_OP,_d>::operator -- ( int )
  *\brief Move to the previous node in the octree that satisfied the traversal criteria.
  */

