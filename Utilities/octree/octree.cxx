// Included from octree

/**\var template< typename T_, int d_, typename A_ > \
  *     typedef T_ octree<T_,d_,A_>::value_type
  *\brief Shorthand for the application-specific datatype.
  */

/**\var template< typename T_, int d_, typename A_ > \
  *     typedef T_* octree<T_,d_,A_>::pointer
  *\brief Shorthand for a pointer to application-specific data.
  */

/**\var template< typename T_, int d_, typename A_ > \
  *     typedef T_& octree<T_,d_,A_>::reference
  *\brief Shorthand for a reference to application-specific data.
  */

/**\var template< typename T_, int d_, typename A_ > \
  *     typedef const T_* octree<T_,d_,A_>::const_pointer
  *\brief Shorthand for a pointer to immutable application-specific data.
  */

/**\var template< typename T_, int d_, typename A_ > \
  *     typedef const T_& octree<T_,d_,A_>::const_reference
  *\brief Shorthand for a reference to immutable application-specific data.
  */

/**\var template< typename T_, int d_, typename A_ > \
  *     typedef octree<T_,d_,A_> octree<T_,d_,A_>::_self_type
  *\brief Shorthand for the datatype of this class.
  */

/**\var template< typename T_, int d_, typename A_ > \
  *     typedef _self_type* octree<T_,d_,A_>::_self_pointer
  *\brief Shorthand for a pointer to an object of this class.
  */

/**\var template< typename T_, int d_, typename A_ > \
  *     typedef octree_node<T_,d_,A_>* octree<T_,d_,A_>::octree_node_pointer
  *\brief Shorthand for a pointer to a node contained by the octree.
  */

/**\var template< typename T_, int d_, typename A_ > \
  *     typedef octree_node<T_,d_,A_>& octree<T_,d_,A_>::octree_node_reference
  *\brief Shorthand for a reference to a node contained by the octree.
  */

/**\var template< typename T_, int d_, typename A_ > \
  *     typedef const octree_node<T_,d_,A_>* octree<T_,d_,A_>::const_octree_node_pointer
  *\brief Shorthand for a pointer to an immutable node contained by the octree.
  */

/**\var template< typename T_, int d_, typename A_ > \
  *     typedef const octree_node<T_,d_,A_>& octree<T_,d_,A_>::const_octree_node_reference
  *\brief Shorthand for a reference to an immutable node contained by the octree.
  */

/**\var template< typename T_, int d_, typename A_ > \
  *     typedef A_ octree<T_,d_,A_>::allocator_type
  *\brief Shorthand for an allocator to be used by this class.
  */

/**\var template< typename T_, int d_, typename A_ > \
  *     typedef octree_iterator< T_, T_&, T_*, _self_type, _self_pointer, d_ > octree<T_,d_,A_>::iterator
  *\brief Shorthand for an iterator that traverses the nodes contained in the octree.
  */

/**\var template< typename T_, int d_, typename A_ > \
  *     typedef octree_iterator< T_, const T_&, const T_*, _self_type, _self_pointer, d_ > octree<T_,d_,A_>::const_iterator
  *\brief Shorthand for an iterator that traverses the nodes contained in the octree in a read-only manner.
  */

/**\var template< typename T_, int d_, typename A_ > \
  *     typedef octree_cursor< T_, T_&, T_*, _self_type, _self_pointer, d_ > octree<T_,d_,A_>::cursor
  *\brief Shorthand for a cursor that traverses the nodes contained in the octree.
  */

/**\var template< typename T_, int d_, typename A_ > \
  *     typedef octree_cursor< T_, const T_&, const T_*, _self_type, _self_pointer, d_ > octree<T_,d_,A_>::const_cursor
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
template< typename T_, int d_, typename A_ >
octree<T_,d_,A_>::octree( const double* center, double length )
{
  this->_M_root = new octree_node<T_,d_,A_>( this, center, length );
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
template< typename T_, int d_, typename A_ >
octree<T_,d_,A_>::octree( const double* center, double length, const value_type& value )
{
  this->_M_root = new octree_node<T_,d_,A_>( this, center, length, value );
}

/**\brief Octree destructor.
  *
  * Deletes the octree nodes and any application-specific data stored with them.
  */
template< typename T_, int d_, typename A_ >
octree<T_,d_,A_>::~octree()
{
  delete this->_M_root;
}

/**\fn template< typename T_, int d_, typename A_ > \
  *    octree_node_pointer octree<T_,d_,A_>::root()
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
template< typename T_, int d_, typename A_ >
size_t octree<T_,d_,A_>::size( bool only_leaves )
{
  size_t number = 0;
  iterator it;
  for ( it = this->begin( only_leaves ); it != this->end(); ++it )
    ++number;
  return number;
}

/**\var template< typename T_, int d_, typename A_ > \
  *     octree_node_pointer octree<T_,d_,A_>::_M_root
  *\brief The root (top-level) node of the octree.
  */

