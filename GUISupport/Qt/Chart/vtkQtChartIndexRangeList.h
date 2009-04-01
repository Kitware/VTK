/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtChartIndexRangeList.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

/// \file vtkQtChartIndexRangeList.h
/// \date March 26, 2009

#ifndef _vtkQtChartIndexRangeList_h
#define _vtkQtChartIndexRangeList_h

#include "vtkQtChartExport.h"


/// \class vtkQtChartIndexRange
/// \brief
///   The vtkQtChartIndexRange class is used to build a tree of index
///   ranges.
class VTKQTCHART_EXPORT vtkQtChartIndexRange
{
public:
  vtkQtChartIndexRange();

  /// \brief
  ///   Creates an index range instance.
  /// \param first The first index in the range.
  /// \param second The last index in the range.
  /// \param black True if the node should be black.
  vtkQtChartIndexRange(int first, int second, bool black=true);
  vtkQtChartIndexRange(const vtkQtChartIndexRange &other);
  ~vtkQtChartIndexRange();

  vtkQtChartIndexRange &operator=(const vtkQtChartIndexRange &other);

  /// \name Index Range Methods
  //@{
  /// \brief
  ///   Gets the first index in the range.
  /// \return
  ///   The first index in the range.
  int getFirst() const {return this->First;}

  /// \brief
  ///   Sets the first index in the range.
  ///
  /// This method also updates the first index range for all the
  /// affected parent nodes.
  ///
  /// \param first The first index in the range.
  void setFirst(int first);

  /// \brief
  ///   Gets the last index in the range.
  /// \return
  ///   The last index in the range.
  int getSecond() const {return this->Second;}

  /// \brief
  ///   Sets the last index in the range.
  ///
  /// This method also updates the last index range for all the
  /// affected parent nodes.
  ///
  /// \param second The last index in the range.
  void setSecond(int second);

  /// \brief
  ///   Gets whether or not the given value is in the range.
  /// \param value The value to find.
  /// \return
  ///   True if the given value is in the range.
  bool contains(int value) const;
  //@}

  /// \name Tree Methods
  //@{
  /// \brief
  ///   Gets whether the node is black or red.
  /// \return
  ///   True if the node is black. False if the node is red.
  bool isBlack() const {return this->Black;}

  /// \brief
  ///   Sets whether the node is black or red.
  /// \param black True if the node is black. False if the node is red.
  void setBlack(bool black) {this->Black = black;}

  /// \brief
  ///   Gets the parent node.
  /// \return
  ///   A pointer to the parent node.
  vtkQtChartIndexRange *getParent() const {return this->Parent;}

  /// \brief
  ///   Sets the parent node.
  /// \param parent The new parent node.
  void setParent(vtkQtChartIndexRange *parent) {this->Parent = parent;}

  /// \brief
  ///   Gets the left child node.
  /// \return
  ///   A pointer to the left child node.
  vtkQtChartIndexRange *getLeft() const {return this->Left;}

  /// \brief
  ///   Sets the left child node.
  /// \param left The new left child node.
  void setLeft(vtkQtChartIndexRange *left) {this->Left = left;}

  /// \brief
  ///   Gets the right child node.
  /// \return
  ///   A pointer to the right child node.
  vtkQtChartIndexRange *getRight() const {return this->Right;}

  /// \brief
  ///   Sets the right child node.
  /// \param right The new right child node.
  void setRight(vtkQtChartIndexRange *right) {this->Right = right;}
  //@}

private:
  vtkQtChartIndexRange *Parent; ///< Stores the parent node.
  vtkQtChartIndexRange *Left;   ///< Stores the left child.
  vtkQtChartIndexRange *Right;  ///< Stores the right child.
  bool Black;                   ///< Used to balance the tree.
  int First;                    ///< Stores the range's first index.
  int Second;                   ///< Stores the range's last index.
};


/// \class vtkQtChartIndexRangeList
/// \brief
///   The vtkQtChartIndexRangeList class stores a searchable list of
///   index ranges.
///
/// A modified red-black binary tree is used to store the ranges. The
/// red/black flag on each node is used to keep the tree balanced. Each
/// parent node holds the combined range of the two children. The leaf
/// nodes contain the list of actual ranges. The navigation and search
/// methods provide an interface to the leaf nodes.
class VTKQTCHART_EXPORT vtkQtChartIndexRangeList
{
public:
  vtkQtChartIndexRangeList();

  /// \brief
  ///   Creates an index range list instance with one range.
  /// \param first The first index in the range.
  /// \param second The last index in the range.
  vtkQtChartIndexRangeList(int first, int second);
  vtkQtChartIndexRangeList(const vtkQtChartIndexRangeList &other);
  ~vtkQtChartIndexRangeList();

  vtkQtChartIndexRangeList &operator=(const vtkQtChartIndexRangeList &other);

  /// \name Navigation Methods
  //@{
  /// \brief
  ///   Gets whether or not the list is empty.
  /// \return
  ///   True if the list is empty.
  bool isEmpty() const {return this->Root == 0;}

  /// \brief
  ///   Gets the first index range in the list.
  /// \return
  ///   A pointer to the first index range or null if the list is empty.
  vtkQtChartIndexRange *getFirst() const;

  /// \brief
  ///   Gets the last index range in the list.
  /// \return
  ///   A pointer to the last index range or null if the list is empty.
  vtkQtChartIndexRange *getLast() const;

  /// \brief
  ///   Gets the next index range in the list.
  /// \param range The index range to search from.
  /// \return
  ///   A pointer to the next index range in the list or null if there
  ///   are no more nodes in the list.
  vtkQtChartIndexRange *getNext(vtkQtChartIndexRange *range) const;

  /// \brief
  ///   Gets the previous index range in the list.
  /// \param range The index range to search from.
  /// \return
  ///   A pointer to the previous index range in the list or null if
  ///   there are no more nodes in the list.
  vtkQtChartIndexRange *getPrevious(vtkQtChartIndexRange *range) const;
  //@}

  /// \name Search Methods
  //@{
  /// \brief
  ///   Finds the closest index range to the given value.
  /// \param value The value to search for.
  /// \return
  ///   A pointer to the nearest index range or null if the list is
  ///   empty.
  vtkQtChartIndexRange *findClosest(int value) const;

  /// \brief
  ///   Gets whether or not the given value is contained in the list.
  /// \return
  ///   True if the given value is contained in the list.
  bool contains(int value) const;
  //@}

  /// \name Modification Methods
  //@{
  /// \brief
  ///   Clears the list of index ranges.
  /// \return
  ///   True if the list was modified.
  bool clear();

  /// \brief
  ///   Sets the list to one index range.
  /// \param first The first index in the range.
  /// \param second The last index in the range.
  /// \return
  ///   True if the list was modified.
  bool setRange(int first, int second);

  /// \brief
  ///   Copies the given list of index ranges.
  /// \param ranges The list of index ranges to copy.
  /// \return
  ///   True if the list was modified.
  bool setRanges(const vtkQtChartIndexRangeList &ranges);

  /// \brief
  ///   Adds the given index range to the list.
  /// \param first The first index in the range.
  /// \param second The last index in the range.
  /// \return
  ///   True if the list was modified.
  bool addRange(int first, int second);

  /// \brief
  ///   Adds the list of index ranges to the list.
  /// \param ranges The list of index ranges to add.
  /// \return
  ///   True if the list was modified.
  bool addRanges(const vtkQtChartIndexRangeList &ranges);

  /// \brief
  ///   Subtracts the given index range from the list.
  /// \param first The first index in the range.
  /// \param second The last index in the range.
  /// \return
  ///   True if the list was modified.
  bool subtractRange(int first, int second);

  /// \brief
  ///   Subtracts the list of index ranges to the list.
  /// \param ranges The list of index ranges to subtract.
  /// \return
  ///   True if the list was modified.
  bool subtractRanges(const vtkQtChartIndexRangeList &ranges);

  /// \brief
  ///   Sets the unique index ranges from the current list and the
  ///   given range.
  /// \param first The first index in the range.
  /// \param second The last index in the range.
  /// \return
  ///   True if the list was modified.
  bool xorRange(int first, int second);

  /// \brief
  ///   Sets the unique index ranges from the current list and the
  ///   given list.
  /// \param ranges The list of index ranges.
  /// \return
  ///   True if the list was modified.
  bool xorRanges(const vtkQtChartIndexRangeList &ranges);

  /// \brief
  ///   Limits the list of index ranges to the given bounds.
  /// \param minimum The minimum boundary of indexes.
  /// \param maximum The maximum boundary of indexes.
  void limitRange(int minimum, int maximum);

  /// \brief
  ///   Offsets the indexes in the list.
  ///
  /// The indexes are only modified if they are greater than or equal
  /// to the given start index.
  ///
  /// \param start The index to start changes.
  /// \param offset The amount added to each index. Use a negative
  ///   value to subtract from each index.
  /// \return
  ///   True if the list was modified.
  bool offsetRanges(int start, int offset);
  //@}

private:
  /// \brief
  ///   Finds the bounding leaf node for the given value.
  /// \param value The value to search for.
  /// \param left True if the leaf should bound the value on the left.
  ///   False if the leaf should bound the value on the right.
  /// \return
  ///   A pointer to the bounding leaf node.
  vtkQtChartIndexRange *findNode(int value, bool left) const;

  /// \brief
  ///   Rotates the given node left in the tree.
  ///
  /// The tree root and the ranges are updated as well.
  ///
  /// \param node The root of the rotation.
  void rotateLeft(vtkQtChartIndexRange *node);

  /// \brief
  ///   Rotates the given node right in the tree.
  ///
  /// The tree root and the ranges are updated as well.
  ///
  /// \param node The root of the rotation.
  void rotateRight(vtkQtChartIndexRange *node);

  /// \brief
  ///   Inserts a new leaf node into the tree.
  ///
  /// A new parent node is inserted in place of the current node. The
  /// current node and the new node are inserted as children of the
  /// parent node. The nodes are ordered according to the \c left
  /// parameter. The tree is rebalance after the insertion.
  ///
  /// \param current The current tree node to insert near.
  /// \param node The new leaf node.
  /// \param left True if the new node should be inserted left of the
  ///   current node. False to be inserted to the right.
  void insertNode(vtkQtChartIndexRange *current, vtkQtChartIndexRange *node,
      bool left);

  /// \brief
  ///   Removes the given leaf node from the tree.
  ///
  /// The parent node is also deleted since it is no longer needed.
  /// The node's sibling takes the place of the parent in the tree.
  /// The tree is rebalanced after removing the nodes.
  ///
  /// \param node The leaf node to remove.
  /// \note
  ///   The node should be deleted after calling this method.
  void removeNode(vtkQtChartIndexRange *node);

  /// \brief
  ///   Removes all the leaf nodes between the given nodes.
  /// \param left The left-most leaf node in the range.
  /// \param right The right-most leaf node in the range.
  int removeBetween(vtkQtChartIndexRange *left, vtkQtChartIndexRange *right);

  /// \brief
  ///   Gets the next node in the tree.
  /// \param node The starting node.
  /// \return
  ///   A pointer to the next node in the tree.
  vtkQtChartIndexRange *getNextNode(vtkQtChartIndexRange *node) const;

  /// \brief
  ///   Gets the previous node in the tree.
  /// \param node The starting node.
  /// \return
  ///   A pointer to the previous node in the tree.
  vtkQtChartIndexRange *getPreviousNode(vtkQtChartIndexRange *node) const;

  /// \brief
  ///   Gets the left-most descendant in the given sub-tree.
  /// \param root The root of the sub-tree to search.
  /// \return
  ///   A pointer to the left-most descendant in the given sub-tree.
  vtkQtChartIndexRange *getFirstNode(vtkQtChartIndexRange *root) const;

  /// \brief
  ///   Gets the right-most descendant in the given sub-tree.
  /// \param root The root of the sub-tree to search.
  /// \return
  ///   A pointer to the right-most descendant in the given sub-tree.
  vtkQtChartIndexRange *getLastNode(vtkQtChartIndexRange *root) const;

private:
  vtkQtChartIndexRange *Root; ///< Stores the tree root.
};

#endif
