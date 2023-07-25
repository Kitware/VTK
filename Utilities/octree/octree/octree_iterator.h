// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#ifndef octree_iterator_
#define octree_iterator_

// Included by octree

#include <iterator>

/**\brief An octree iterator.
 *
 * Unlike most STL container iterators, octree iterators contain some state that most developers
 *will be interested in. Specifically, the iterator may be set to traverse siblings but not
 *children, only leaf nodes, or all nodes in an octree. Also, the bounding box of an octree node is
 *implicit in its position in the hierarchy; since octree nodes themselves do not contain their
 *position relative to their parents it is up to the iterator to provide bounding box information.
 *
 *\bug Const octree iterators don't seem to work -- comparison operators aren't defined properly?
 */
VTK_ABI_NAMESPACE_BEGIN
template <typename T_, typename R_, typename P_, typename O_, typename OP_, int d_ = 3>
class octree_iterator : public octree_path<T_, R_, P_, O_, OP_, d_>
{
public:
  typedef O_ octree_type;
  typedef OP_ octree_pointer;
  typedef typename O_::allocator_type octree_allocator_type;
  typedef typename O_::octree_node_reference octree_node_reference;
  typedef typename O_::octree_node_pointer octree_node_pointer;

  typedef std::bidirectional_iterator_tag iterator_category;

  typedef octree_iterator<T_, T_&, T_*, O_, O_*, d_> iterator;
  typedef octree_iterator<T_, const T_&, const T_*, O_, const O_*, d_> const_iterator;
  typedef octree_iterator<T_, R_, P_, O_, OP_, d_> self_iterator;

  bool m_immediate_family; // Iterate over all the subnodes or just the direct children?
  bool m_only_leaf_nodes;  // Should the iterator visit all nodes or only leaf nodes?

  octree_iterator();
  octree_iterator(octree_node_pointer oroot, octree_node_pointer onode, bool only_leaves = true);
  octree_iterator(const const_iterator& it);

  octree_node_pointer check_incr();
  octree_node_pointer check_decr();
  void immediate_family(bool state);
  bool immediate_family() const { return this->m_immediate_family; }
  bool& immediate_family() { return this->m_immediate_family; }

  self_iterator& operator++()
  {
    this->m_current_node = check_incr();
    return *this;
  }
  self_iterator operator++(int)
  {
    self_iterator tmp = *this;
    this->m_current_node = check_incr();
    return tmp;
  }

  self_iterator& operator--()
  {
    this->m_current_node = check_decr();
    return *this;
  }
  self_iterator operator--(int)
  {
    self_iterator tmp = *this;
    this->m_current_node = check_decr();
    return tmp;
  }
};

VTK_ABI_NAMESPACE_END
#endif // octree_iterator_
