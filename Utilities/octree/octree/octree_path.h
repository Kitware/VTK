// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#ifndef octree_path_
#define octree_path_

// Included by octree

/**\brief An octree path.
 *
 * A path is like an iterator without the capability to perform linear traversal.
 */
VTK_ABI_NAMESPACE_BEGIN
template <typename T_, typename R_, typename P_, typename O_, typename OP_, int d_ = 3>
class octree_path
{
public:
  typedef O_ octree_type;
  typedef OP_ octree_pointer;
  typedef typename O_::allocator_type octree_allocator_type;
  typedef typename O_::octree_node_reference octree_node_reference;
  typedef typename O_::octree_node_pointer octree_node_pointer;
  typedef typename std::vector<octree_node_pointer>::size_type size_type;

  typedef octree_path<T_, T_&, T_*, O_, O_*, d_> path;
  typedef octree_path<T_, const T_&, const T_*, O_, const O_*, d_> const_path;
  typedef octree_path<T_, R_, P_, O_, OP_, d_> self_path;

  octree_node_pointer m_root;                 // The root of the octree we are iterating over
  std::vector<octree_node_pointer> m_parents; // List of parent nodes
  std::vector<int> m_indices;                 // List of parent child indices
  octree_node_pointer m_current_node;         // Current path head

  octree_path();
  octree_path(octree_pointer otree);
  octree_path(octree_node_pointer oroot);
  octree_path(octree_node_pointer oroot, std::vector<int>& children);

  octree_node_reference operator*() const { return *m_current_node; }
  octree_node_pointer operator->() const { return &(operator*()); }

  size_type level() const { return this->m_parents.size(); }

  bool operator==(const path& it)
  {
    return m_root == it.m_root && m_current_node == it.m_current_node;
  }
  bool operator==(const const_path& it)
  {
    return m_root == it.m_root && m_current_node == it.m_current_node;
  }

  bool operator!=(const path& it)
  {
    return m_root != it.m_root || m_current_node != it.m_current_node;
  }
  bool operator!=(const const_path& it)
  {
    return m_root != it.m_root || m_current_node != it.m_current_node;
  }
};

VTK_ABI_NAMESPACE_END
#endif // octree_path_
