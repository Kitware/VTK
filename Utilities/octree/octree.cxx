// Included from octree

/**\var template< typename _T, int _d, typename _A > \
  *     typedef _T octree<_T,_d,_A>::value_type
  *\brief Shorthand for the application-specific datatype.
  */

/**\var template< typename _T, int _d, typename _A > \
  *     typedef _T* octree<_T,_d,_A>::pointer
  *\brief Shorthand for a pointer to application-specific data.
  */

/**\var template< typename _T, int _d, typename _A > \
  *     typedef _T& octree<_T,_d,_A>::reference
  *\brief Shorthand for a reference to application-specific data.
  */

/**\var template< typename _T, int _d, typename _A > \
  *     typedef const _T* octree<_T,_d,_A>::const_pointer
  *\brief Shorthand for a pointer to immutable application-specific data.
  */

/**\var template< typename _T, int _d, typename _A > \
  *     typedef const _T& octree<_T,_d,_A>::const_reference
  *\brief Shorthand for a reference to immutable application-specific data.
  */

/**\var template< typename _T, int _d, typename _A > \
  *     typedef octree<_T,_d,_A> octree<_T,_d,_A>::_self_type
  *\brief Shorthand for the datatype of this class.
  */

/**\var template< typename _T, int _d, typename _A > \
  *     typedef _self_type* octree<_T,_d,_A>::_self_pointer
  *\brief Shorthand for a pointer to an object of this class.
  */

/**\var template< typename _T, int _d, typename _A > \
  *     typedef octree_node<_T,_d,_A>* octree<_T,_d,_A>::octree_node_pointer
  *\brief Shorthand for a pointer to a node contained by the octree.
  */

/**\var template< typename _T, int _d, typename _A > \
  *     typedef octree_node<_T,_d,_A>& octree<_T,_d,_A>::octree_node_reference
  *\brief Shorthand for a reference to a node contained by the octree.
  */

/**\var template< typename _T, int _d, typename _A > \
  *     typedef const octree_node<_T,_d,_A>* octree<_T,_d,_A>::const_octree_node_pointer
  *\brief Shorthand for a pointer to an immutable node contained by the octree.
  */

/**\var template< typename _T, int _d, typename _A > \
  *     typedef const octree_node<_T,_d,_A>& octree<_T,_d,_A>::const_octree_node_reference
  *\brief Shorthand for a reference to an immutable node contained by the octree.
  */

/**\var template< typename _T, int _d, typename _A > \
  *     typedef _A octree<_T,_d,_A>::allocator_type
  *\brief Shorthand for an allocator to be used by this class.
  */

/**\var template< typename _T, int _d, typename _A > \
  *     typedef octree_iterator< _T, _T&, _T*, _self_type, _self_pointer, _d > octree<_T,_d,_A>::iterator
  *\brief Shorthand for an iterator that traverses the nodes contained in the octree.
  */

/**\var template< typename _T, int _d, typename _A > \
  *     typedef octree_iterator< _T, const _T&, const _T*, _self_type, _self_pointer, _d > octree<_T,_d,_A>::const_iterator
  *\brief Shorthand for an iterator that traverses the nodes contained in the octree in a read-only manner.
  */

/**\var template< typename _T, int _d, typename _A > \
  *     typedef octree_cursor< _T, _T&, _T*, _self_type, _self_pointer, _d > octree<_T,_d,_A>::cursor
  *\brief Shorthand for a cursor that traverses the nodes contained in the octree.
  */

/**\var template< typename _T, int _d, typename _A > \
  *     typedef octree_cursor< _T, const _T&, const _T*, _self_type, _self_pointer, _d > octree<_T,_d,_A>::const_cursor
  *\brief Shorthand for a cursor that traverses the nodes contained in the octree in a read-only manner.
  */

/**\brief Octree constructor (deprecated).
  *
  * There is no default constructor because the size of the octree must be fixed at construction time.
  * If it were not fixed, then someone could change the root node's bounds later and (1) its
  * children would be inconsistently bounded and/or (2) any application-specific data dependent on the
  * geometry would be incorrect.
  *
  * Because this version does not properly initialize application-specific data at the root node, it is deprecated.
  *
  * @param[in] center An array of coordinates specifying the center of the octree
  * @param[in] length The length (size) of each side of the octree
  */
template< typename _T, int _d, typename _A >
octree<_T,_d,_A>::octree( const double* center, double length )
{
  this->_M_root = new octree_node<_T,_d,_A>( this, center, length );
}

/**\brief Octree constructor.
  *
  * There is no default constructor because the size of the octree must be fixed at construction time.
  * If it were not fixed, then someone could change the root node's bounds later and (1) its
  * children would be inconsistently bounded and/or (2) any application-specific data dependent on the
  * geometry would be incorrect.
  *
  * This version takes a reference to application-specific data which is used to properly initialize
  * the root node's value. This is the preferred constructor.
  *
  * @param[in] center An array of coordinates specifying the center of the octree
  * @param[in] length The length (size) of each side of the octree
  * @param[in] value  Application-specific data to store at the root node
  */
template< typename _T, int _d, typename _A >
octree<_T,_d,_A>::octree( const double* center, double length, const value_type& value )
{
  this->_M_root = new octree_node<_T,_d,_A>( this, center, length, value );
}

/**\brief Octree destructor.
  *
  * Deletes the octree nodes and any application-specific data stored with them.
  */
template< typename _T, int _d, typename _A >
octree<_T,_d,_A>::~octree()
{
  delete this->_M_root;
}

/**\fn template< typename _T, int _d, typename _A > \
  *    octree_node_pointer octree<_T,_d,_A>::root()
  *\brief Returns the root (top-level) node of the octree.
  */

/**\brief Return the number of nodes in the octree (or optionally leaf nodes)
  *
  * By default, this will return the count of all the nodes in the tree -- not just leaf nodes.
  * If you set \a only_leaves to true, you will receive a count of just the leaf nodes.
  * Note that this is not the same as the default for iteration!
  *
  *\warning This is not a fast routine; it traverses the entire tree to count nodes.
  */
template< typename _T, int _d, typename _A >
size_t octree<_T,_d,_A>::size( bool only_leaves )
{
  size_t number = 0;
  iterator it;
  for ( it = this->begin( only_leaves ); it != this->end(); ++it )
    ++number;
  return number;
}

/**\var template< typename _T, int _d, typename _A > \
  *     octree_node_pointer octree<_T,_d,_A>::_M_root
  *\brief The root (top-level) node of the octree.
  */

