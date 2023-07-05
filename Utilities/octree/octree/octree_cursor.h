// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#ifndef octree_cursor_
#define octree_cursor_
// Included by octree

/**\brief An octree cursor.
 *
 * Unlike an iterator, an octree cursor does not provide an order in which to traverse nodes.
 * Instead, it provides 2 primitive operations: up() and down() that can be used to
 * traverse the tree in any order.
 * There are also some utility routines named over(), axis_partner(), and neighbor() that
 * call up() followed by down() with specific arguments to help traverse nodes that are
 * children of the same parent.
 *
 * A cursor contains no storage beyond its base class, octree_path, so you may assign to a
 * cursor from any descendant of octree_path including octree_iterator.
 */
VTK_ABI_NAMESPACE_BEGIN
template <typename T_, typename R_, typename P_, typename O_, typename OP_, int d_ = 3>
class octree_cursor : public octree_path<T_, R_, P_, O_, OP_, d_>
{
public:
  typedef O_ octree_type;
  typedef OP_ octree_pointer;
  typedef typename O_::allocator_type octree_allocator_type;
  typedef typename O_::octree_node_reference octree_node_reference;
  typedef typename O_::octree_node_pointer octree_node_pointer;

  typedef octree_path<T_, T_&, T_*, O_, O_*, d_> path;
  typedef octree_path<T_, const T_&, const T_*, O_, const O_*, d_> const_path;
  typedef octree_path<T_, R_, P_, O_, OP_, d_> self_path;

  typedef octree_cursor<T_, T_&, T_*, O_, O_*, d_> cursor;
  typedef octree_cursor<T_, const T_&, const T_*, O_, const O_*, d_> const_cursor;
  typedef octree_cursor<T_, R_, P_, O_, OP_, d_> self_cursor;

  octree_cursor();
  octree_cursor(octree_pointer otree);
  octree_cursor(octree_node_pointer oroot);
  octree_cursor(const const_path& src);

  void up();
  void down(int child_of_this_node);

  int where() const;

  void over(int child_of_shared_parent);
  void axis_partner(int axis);
  bool axis_bit(int axis) const;

  bool visit(const std::vector<int>& path);

  self_path& operator=(const path& it);
  self_path& operator=(const const_path& it);
};

VTK_ABI_NAMESPACE_END
#endif // octree_cursor_
