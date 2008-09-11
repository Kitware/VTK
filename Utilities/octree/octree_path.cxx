// Included by octree

/**\typedef template<typename _T,typename _R,typename _P,typename _O,typename _OP,int _d> \
  *         typedef _O octree_path<_T,_R,_P,_O,_OP,_d>::octree_type;
  * \brief Shorthand for an octree over which this class can iterate.
  */

/**\typedef template<typename _T,typename _R,typename _P,typename _O,typename _OP,int _d> \
  *         typedef _OP octree_path<_T,_R,_P,_O,_OP,_d>::octree_pointer;
  * \brief Shorthand for a pointer to an octree over which this class can iterate.
  */

/**\typedef template<typename _T,typename _R,typename _P,typename _O,typename _OP,int _d> \
  *         typedef typename _O::allocator_type octree_path<_T,_R,_P,_O,_OP,_d>::octree_allocator_type;
  * \brief Shorthand for the allocator used by the octrees over which this class iterates.
  */

/**\typedef template<typename _T,typename _R,typename _P,typename _O,typename _OP,int _d> \
  *         typedef typename _O::octree_node_reference octree_path<_T,_R,_P,_O,_OP,_d>::octree_node_reference;
  * \brief Shorthand for a reference to a node in the octree.
  */

/**\typedef template<typename _T,typename _R,typename _P,typename _O,typename _OP,int _d> \
  *         typedef typename _O::octree_node_pointer octree_path<_T,_R,_P,_O,_OP,_d>::octree_node_pointer;
  * \brief Shorthand for a pointer to a node in the octree.
  */

/**\typedef template<typename _T,typename _R,typename _P,typename _O,typename _OP,int _d> \
  *         typedef octree_path< _T, _T&, _T*, _O, _O*, _d > octree_path<_T,_R,_P,_O,_OP,_d>::path;
  * \brief Shorthand for a non-const octree path (regardless of whether the current path is const or not).
  */

/**\typedef template<typename _T,typename _R,typename _P,typename _O,typename _OP,int _d> \
  *         typedef octree_path< _T, const _T&, const _T*, _O, const _O*, _d > octree_path<_T,_R,_P,_O,_OP,_d>::const_path;
  * \brief Shorthand for a const octree path (regardless of whether the current path is const or not).
  */

/**\typedef template<typename _T,typename _R,typename _P,typename _O,typename _OP,int _d> \
  *         typedef octree_path< _T, _R, _P, _O, _OP, _d > octree_path<_T,_R,_P,_O,_OP,_d>::self_path;
  * \brief Shorthand for an path of the same type as the current path (be it const or not).
  */

/**\var template<typename _T,typename _R,typename _P,typename _O,typename _OP,int _d> \
  *     octree_pointer octree_path<_T,_R,_P,_O,_OP,_d>::_M_octree
  *\brief The octree we are iterating over
  */

/**\var template<typename _T,typename _R,typename _P,typename _O,typename _OP,int _d> \
  *     vtkstd::vector<octree_node_pointer> octree_path<_T,_R,_P,_O,_OP,_d>::_M_parents
  *\brief List of parent nodes
  */

/**\var template<typename _T,typename _R,typename _P,typename _O,typename _OP,int _d> \
  *     vtkstd::vector<int> octree_path<_T,_R,_P,_O,_OP,_d>::_M_indices
  *\brief List of parent child indices
  */

/**\var template<typename _T,typename _R,typename _P,typename _O,typename _OP,int _d> \
  *     octree_node_pointer octree_path<_T,_R,_P,_O,_OP,_d>::_M_current_node
  *\brief Current node at the head of the path
  */

/**\brief Default constructor.
  */
template< typename _T, typename _R, typename _P, typename _O, typename _OP, int _d >
octree_path<_T,_R,_P,_O,_OP,_d>::octree_path()
{
  this->_M_octree = 0;
  this->_M_current_node = 0;
}

/**\brief Simplest valid constructor.
  *
  * This creates a path that points to the root node of the specified \a tree.
  */
template< typename _T, typename _R, typename _P, typename _O, typename _OP, int _d >
octree_path<_T,_R,_P,_O,_OP,_d>::octree_path( octree_pointer tree )
{
  this->_M_octree = tree;
  this->_M_current_node = tree->root();
}

/**\brief Flexible constructor.
  *
  * This creates a path that points to a particular node of the specified \a tree,
  * given a \a path of nodes to descend from the root of the \a tree.
  */
template< typename _T, typename _R, typename _P, typename _O, typename _OP, int _d >
octree_path<_T,_R,_P,_O,_OP,_d>::octree_path( octree_pointer tree, vtkstd::vector<int>& children )
{
  this->_M_octree = tree;
  this->_M_current_node = tree->root();
  for ( vtkstd::vector<int>::iterator cit = children.begin(); cit != children.end(); ++cit )
    {
    this->_M_parents.push_back( this->_M_current_node );
    this->_M_indices.push_back( *cit );
    this->_M_current_node = (*this->_M_current_node)[*cit];
    }
}

/**\brief Destructor.
  *
  */
template< typename _T, typename _R, typename _P, typename _O, typename _OP, int _d >
octree_path<_T,_R,_P,_O,_OP,_d>::~octree_path()
{
}

/**\fn template< typename _T, typename _R, typename _P, typename _O, typename _OP, int _d > \
  *    octree_node_reference octree_path<_T,_R,_P,_O,_OP,_d>::operator * () const
  *\brief Provide access to the node at the current path head.
  */

/**\fn template< typename _T, typename _R, typename _P, typename _O, typename _OP, int _d > \
  *    octree_node_pointer octree_path<_T,_R,_P,_O,_OP,_d>::operator -> () const
  *\brief Provide access to the node at the current path head.
  */

/**\fn template< typename _T, typename _R, typename _P, typename _O, typename _OP, int _d > \
  *    int octree_path<_T,_R,_P,_O,_OP,_d>::level() const
  *\brief Return the depth of the current path.
  *
  * The root node of the octree is at level 0. Its children are all at level 1. Their children are at level 2,
  * and so forth.
  */


/**\brief Assignment operator (for copying paths of mutable nodes).
  *
  */
template< typename _T, typename _R, typename _P, typename _O, typename _OP, int _d >
octree_path<_T,_R,_P,_O,_OP,_d>& octree_path<_T,_R,_P,_O,_OP,_d>::operator = ( const path& src )
{
  this->_M_octree = src._M_octree;
  this->_M_parents = src._M_parents;
  this->_M_indices = src._M_indices;
  this->_M_current_node = src._M_current_node;
  return *this;
}

/**\fn    template< typename _T, typename _R, typename _P, typename _O, typename _OP, int _d > \
  *       self_path& octree_path<_T,_R,_P,_O,_OP,_d>::operator = ( const const_path& src )
  *\brief Assignment operator (for copying paths of immutable nodes).
  * Frappy
  */

/**\brief Assignment operator (for copying paths of immutable nodes).
  *
  */
template< typename _T, typename _R, typename _P, typename _O, typename _OP, int _d >
octree_path<_T,_R,_P,_O,_OP,_d>& octree_path<_T,_R,_P,_O,_OP,_d>::operator = ( const const_path& src )
{
  this->_M_octree = const_cast<_OP>( src._M_octree );
  this->_M_parents = src._M_parents;
  this->_M_indices = src._M_indices;
  this->_M_current_node = src._M_current_node;
  return *this;
}

/**\fn template< typename _T, typename _R, typename _P, typename _O, typename _OP, int _d > \
  *    bool octree_path<_T,_R,_P,_O,_OP,_d>::operator == ( const path& it )
  *\brief Compare two paths for equality.
  *
  * Iterators are considered equal iff they have the same octree and point to the same node currently.
  * The rest of the paths' state is ignored by the test.
  */

/**\fn template< typename _T, typename _R, typename _P, typename _O, typename _OP, int _d > \
  *    bool octree_path<_T,_R,_P,_O,_OP,_d>::operator == ( const const_path& it )
  *\brief Compare two paths for equality.
  *
  * Iterators are considered equal iff they have the same octree and point to the same node currently.
  * The rest of the paths' state is ignored by the test.
  */

/**\fn template< typename _T, typename _R, typename _P, typename _O, typename _OP, int _d > \
  *    bool octree_path<_T,_R,_P,_O,_OP,_d>::operator != ( const path& it )
  *\brief Compare two paths for inequality.
  *
  * Iterators are considered equal iff they have the same octree and point to the same node currently.
  * The rest of the paths' state is ignored by the test.
  */

/**\fn template< typename _T, typename _R, typename _P, typename _O, typename _OP, int _d > \
  *    bool octree_path<_T,_R,_P,_O,_OP,_d>::operator != ( const const_path& it )
  *\brief Compare two paths for inequality.
  *
  * Iterators are considered equal iff they have the same octree and point to the same node currently.
  * The rest of the paths' state is ignored by the test.
  */

