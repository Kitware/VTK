/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtChartIndexRangeList.cxx

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

/// \file vtkQtChartIndexRangeList.cxx
/// \date March 26, 2009

#ifdef _MSC_VER
// Disable warnings that Qt headers give.
#pragma warning(disable:4127)
#endif

#include "vtkQtChartIndexRangeList.h"

#include <QPair>


//-----------------------------------------------------------------------------
vtkQtChartIndexRange::vtkQtChartIndexRange()
{
  this->Parent = 0;
  this->Left = 0;
  this->Right = 0;
  this->Black = false;
  this->First = -1;
  this->Second = -1;
}

vtkQtChartIndexRange::vtkQtChartIndexRange(int first, int second, bool black)
{
  this->Parent = 0;
  this->Left = 0;
  this->Right = 0;
  this->Black = black;
  if(second < first)
    {
    this->First = second;
    this->Second = first;
    }
  else
    {
    this->First = first;
    this->Second = second;
    }
}

vtkQtChartIndexRange::vtkQtChartIndexRange(const vtkQtChartIndexRange &other)
{
  this->Parent = 0;
  this->Left = 0;
  this->Right = 0;
  this->Black = other.Black;
  this->First = other.First;
  this->Second = other.Second;
}

vtkQtChartIndexRange::~vtkQtChartIndexRange()
{
  // Delete the child nodes.
  if(this->Left)
    {
    delete this->Left;
    }

  if(this->Right)
    {
    delete this->Right;
    }
}

vtkQtChartIndexRange &vtkQtChartIndexRange::operator=(
    const vtkQtChartIndexRange &other)
{
  this->Black = other.Black;
  this->First = other.First;
  this->Second = other.Second;
  return *this;
}

void vtkQtChartIndexRange::setFirst(int first)
{
  if(this->First != first)
    {
    this->First = first;

    // Update the affected parent nodes.
    vtkQtChartIndexRange *node = this;
    while(node)
      {
      vtkQtChartIndexRange *parent = node->Parent;
      if(!parent || node == parent->Right)
        {
        break;
        }

      parent->First = node->First;
      node = parent;
      }
    }
}

void vtkQtChartIndexRange::setSecond(int second)
{
  if(this->Second != second)
    {
    this->Second = second;

    // Update the affected parent nodes.
    vtkQtChartIndexRange *node = this;
    while(node)
      {
      vtkQtChartIndexRange *parent = node->Parent;
      if(!parent || node == parent->Left)
        {
        break;
        }

      parent->Second = node->Second;
      node = parent;
      }
    }
}

bool vtkQtChartIndexRange::contains(int value) const
{
  return value >= this->First && value <= this->Second;
}


//-----------------------------------------------------------------------------
vtkQtChartIndexRangeList::vtkQtChartIndexRangeList()
{
  this->Root = 0;
}

vtkQtChartIndexRangeList::vtkQtChartIndexRangeList(int first, int second)
{
  this->Root = 0;

  this->setRange(first, second);
}

vtkQtChartIndexRangeList::vtkQtChartIndexRangeList(
    const vtkQtChartIndexRangeList &other)
{
  this->Root = 0;

  // Copy the other list of ranges.
  this->setRanges(other);
}

vtkQtChartIndexRangeList::~vtkQtChartIndexRangeList()
{
  this->clear();
}

vtkQtChartIndexRangeList &vtkQtChartIndexRangeList::operator=(
    const vtkQtChartIndexRangeList &other)
{
  this->setRanges(other);
  return *this;
}

vtkQtChartIndexRange *vtkQtChartIndexRangeList::getFirst() const
{
  return this->getFirstNode(this->Root);
}

vtkQtChartIndexRange *vtkQtChartIndexRangeList::getLast() const
{
  return this->getLastNode(this->Root);
}

vtkQtChartIndexRange *vtkQtChartIndexRangeList::getNext(
    vtkQtChartIndexRange *range) const
{
  while(range)
    {
    range = this->getNextNode(range);
    if(range && !range->getLeft() && !range->getRight())
      {
      return range;
      }
    }

  return 0;
}

vtkQtChartIndexRange *vtkQtChartIndexRangeList::getPrevious(
    vtkQtChartIndexRange *range) const
{
  while(range)
    {
    range = this->getPreviousNode(range);
    if(range && !range->getLeft() && !range->getRight())
      {
      return range;
      }
    }

  return 0;
}

vtkQtChartIndexRange *vtkQtChartIndexRangeList::findClosest(int value) const
{
  if(!this->Root)
    {
    return 0;
    }

  if(value < this->Root->getFirst())
    {
    return this->getFirstNode(this->Root);
    }
  else if(value > this->Root->getSecond())
    {
    return this->getLastNode(this->Root);
    }

  vtkQtChartIndexRange *range = this->Root;
  vtkQtChartIndexRange *left = range->getLeft();
  vtkQtChartIndexRange *right = range->getRight();
  while(left || right)
    {
    if(value <= left->getSecond())
      {
      range = left;
      left = range->getLeft();
      right = range->getRight();
      }
    else if(value >= right->getFirst())
      {
      range = right;
      left = range->getLeft();
      right = range->getRight();
      }
    else
      {
      // If the value is between the ranges, get the closest range.
      if(value - left->getSecond() > right->getFirst() - value)
        {
        return this->getFirstNode(right);
        }
      else
        {
        return this->getLastNode(left);
        }
      }
    }

  return range;
}

bool vtkQtChartIndexRangeList::contains(int value) const
{
  if(!this->Root || value < this->Root->getFirst() ||
      value > this->Root->getSecond())
    {
    return false;
    }

  vtkQtChartIndexRange *range = this->Root;
  vtkQtChartIndexRange *left = range->getLeft();
  vtkQtChartIndexRange *right = range->getRight();
  while(left || right)
    {
    if(value <= left->getSecond())
      {
      range = left;
      left = range->getLeft();
      right = range->getRight();
      }
    else if(value >= right->getFirst())
      {
      range = right;
      left = range->getLeft();
      right = range->getRight();
      }
    else
      {
      return false;
      }
    }

  return true;
}

bool vtkQtChartIndexRangeList::clear()
{
  if(this->Root)
    {
    // Clean up the tree nodes.
    delete this->Root;
    this->Root = 0;
    return true;
    }

  return false;
}

bool vtkQtChartIndexRangeList::setRange(int first, int second)
{
  this->clear();

  // Add a new range as the root.
  this->Root = new vtkQtChartIndexRange(first, second);
  return true;
}

bool vtkQtChartIndexRangeList::setRanges(
    const vtkQtChartIndexRangeList &ranges)
{
  bool changed = this->clear();
  if(ranges.isEmpty())
    {
    return changed;
    }

  // Copy the new tree non-recursively.
  this->Root = new vtkQtChartIndexRange(*ranges.Root);
  QList<QPair<vtkQtChartIndexRange *, vtkQtChartIndexRange *> > list;
  QList<QPair<vtkQtChartIndexRange *, vtkQtChartIndexRange *> >::Iterator iter;
  list.append(QPair<vtkQtChartIndexRange *, vtkQtChartIndexRange *>(
      ranges.Root, this->Root));
  while(!list.isEmpty())
    {
    // Copy the children for each pair of nodes (source, target).
    vtkQtChartIndexRange *source = 0;
    vtkQtChartIndexRange *target = 0;
    vtkQtChartIndexRange *copy = 0;
    vtkQtChartIndexRange *child = 0;
    iter = list.begin();
    while(iter != list.end())
      {
      source = iter->first;
      target = iter->second;
      iter = list.erase(iter);
      child = source->getLeft();
      if(child)
        {
        copy = new vtkQtChartIndexRange(*child);
        target->setLeft(copy);
        copy->setParent(target);
        iter = list.insert(iter,
            QPair<vtkQtChartIndexRange *, vtkQtChartIndexRange *>(
            child, copy));
        ++iter;
        }

      child = source->getRight();
      if(child)
        {
        copy = new vtkQtChartIndexRange(*child);
        target->setRight(copy);
        copy->setParent(target);
        iter = list.insert(iter,
            QPair<vtkQtChartIndexRange *, vtkQtChartIndexRange *>(
            child, copy));
        ++iter;
        }
      }
    }

  return true;
}

bool vtkQtChartIndexRangeList::addRange(int first, int second)
{
  // Find the place to add the new range.
  vtkQtChartIndexRange *leaf = this->findNode(first, true);
  if(leaf)
    {
    // Get the leaf closest to the end of the range.
    vtkQtChartIndexRange *leaf2 = this->findNode(second, false);

    // Handle the different range alignment cases.
    if(second < leaf->getFirst() - 1)
      {
      // Insert the new range before the first leaf.
      this->insertNode(leaf, new vtkQtChartIndexRange(first, second), true);
      }
    else if(first > leaf2->getSecond() + 1)
      {
      // Insert the new range after the last leaf.
      this->insertNode(leaf2, new vtkQtChartIndexRange(first, second), false);
      }
    else if(leaf == leaf2)
      {
      // If the leaves are the same, the range is inside the leaf.
      // The range may need to be adjusted if the leaf is first and/or
      // last in the tree.
      bool changed = false;
      if(first < leaf->getFirst())
        {
        leaf->setFirst(first);
        changed = true;
        }

      if(second > leaf->getSecond())
        {
        leaf->setSecond(second);
        changed = true;
        }

      return changed;
      }
    else
      {
      // Delete the leaves between the bounding leaves. They are
      // contained within the new range.
      this->removeBetween(leaf, leaf2);

      // Check if the new range merges with the first leaf.
      if(first <= leaf->getSecond() + 1)
        {
        if(first < leaf->getFirst())
          {
          // Expand the range of the first leaf.
          leaf->setFirst(first);
          }

        // If the new range intersects the last leaf, the last leaf
        // needs to be deleted.
        if(second >= leaf2->getFirst() - 1)
          {
          if(leaf2->getSecond() > second)
            {
            second = leaf2->getSecond();
            }

          this->removeNode(leaf2);
          delete leaf2;
          }

        if(second > leaf->getSecond())
          {
          // Expand the range.
          leaf->setSecond(second);
          }
        }
      else if(second >= leaf2->getFirst() - 1)
        {
        // Expand the range of the last leaf if necessary.
        if(first < leaf2->getFirst())
          {
          leaf2->setFirst(first);
          }

        if(second > leaf2->getSecond())
          {
          leaf2->setSecond(second);
          }
        }
      else
        {
        // Add the new range in between the two leaves.
        this->insertNode(leaf2, new vtkQtChartIndexRange(first, second), true);
        }
      }
    }
  else
    {
    // If there is no closest, the tree is empty.
    this->Root = new vtkQtChartIndexRange(first, second);
    }

  return true;
}

bool vtkQtChartIndexRangeList::addRanges(
    const vtkQtChartIndexRangeList &ranges)
{
  // Add each of the ranges to the tree.
  bool changed = false;
  vtkQtChartIndexRange *range = ranges.getFirst();
  while(range)
    {
    if(this->addRange(range->getFirst(), range->getSecond()))
      {
      changed = true;
      }

    range = ranges.getNext(range);
    }

  return changed;
}

bool vtkQtChartIndexRangeList::subtractRange(int first, int second)
{
  // Find the place to add the new range.
  bool changed = false;
  vtkQtChartIndexRange *leaf = this->findNode(first, true);
  if(leaf)
    {
    // Get the leaf closest to the end of the range.
    vtkQtChartIndexRange *leaf2 = this->findNode(second, false);

    // Handle the different range alignment cases.
    if(second < leaf->getFirst() || first > leaf2->getSecond())
      {
      // The range does not intersect. Do nothing.
      return false;
      }
    else if(leaf == leaf2)
      {
      changed = true;
      if(first <= leaf->getFirst())
        {
        if(second >= leaf->getSecond())
          {
          // The entire leaf range is subtracted. Remove the leaf.
          this->removeNode(leaf);
          delete leaf;
          }
        else
          {
          // Adjust the range of the leaf.
          leaf->setFirst(second + 1);
          }
        }
      else if(second >= leaf->getSecond())
        {
        // Adjust the range of the leaf.
        leaf->setSecond(first - 1);
        }
      else
        {
        // The subtraction takes a slice out of the middle. Add a new
        // node for the right side.
        int temp = leaf->getSecond();
        leaf->setSecond(first - 1);
        first = second + 1;
        second = temp;
        this->insertNode(leaf, new vtkQtChartIndexRange(first, second), false);
        }
      }
    else
      {
      // Remove the leaves between the leaf bounds. They are all in
      // the subtraction range.
      changed = this->removeBetween(leaf, leaf2) > 0;

      // Check if the range intersects the first leaf.
      if(first <= leaf->getFirst())
        {
        // Remove the whole leaf.
        this->removeNode(leaf);
        delete leaf;
        changed = true;
        }
      else if(first <= leaf->getSecond())
        {
        // Adjust the range of the leaf.
        leaf->setSecond(first - 1);
        changed = true;
        }

      // Check if the range intersects the last leaf.
      if(second >= leaf2->getSecond())
        {
        // Remove the whole leaf.
        this->removeNode(leaf2);
        delete leaf2;
        changed = true;
        }
      else if(second >= leaf2->getFirst())
        {
        // Adjust the range of the leaf.
        leaf2->setFirst(second + 1);
        changed = true;
        }
      }
    }

  return changed;
}

bool vtkQtChartIndexRangeList::subtractRanges(
    const vtkQtChartIndexRangeList &ranges)
{
  // Subtract each of the ranges from the tree.
  bool changed = false;
  vtkQtChartIndexRange *range = ranges.getFirst();
  while(range)
    {
    if(this->subtractRange(range->getFirst(), range->getSecond()))
      {
      changed = true;
      }

    range = ranges.getNext(range);
    }

  return changed;
}

bool vtkQtChartIndexRangeList::xorRange(int first, int second)
{
  // Find the place to add the new range.
  vtkQtChartIndexRange *leaf = this->findNode(first, true);
  if(leaf)
    {
    // Get the leaf closest to the end of the range.
    vtkQtChartIndexRange *leaf2 = this->findNode(second, false);

    // Collect all the affected leaves.
    QList<vtkQtChartIndexRange *> list;
    list.append(leaf);
    if(leaf != leaf2)
      {
      vtkQtChartIndexRange *node = this->getNext(leaf);
      while(node != leaf2)
        {
        list.append(node);
        node = this->getNext(node);
        }

      list.append(leaf2);
      }

    // Loop through each of the leaves to handle the different range
    // alignment cases.
    bool doAdd = true;
    QList<vtkQtChartIndexRange *>::Iterator iter = list.begin();
    for( ; iter != list.end(); ++iter)
      {
      if(second < (*iter)->getFirst() - 1)
        {
        // Add the new range.
        this->insertNode(*iter, new vtkQtChartIndexRange(first, second),
            true);
        doAdd = false;
        }
      else if(second == (*iter)->getFirst() - 1)
        {
        // Merge the range with the leaf.
        (*iter)->setFirst(first);
        doAdd = false;
        }
      else if(second > (*iter)->getSecond())
        {
        // The range may intersect with subsequent leaves in this case.
        if(first == (*iter)->getSecond() + 1)
          {
          // The leaf should be merged with the range. The range may
          // insersect subsequent leaves. Remove the leaf and expand
          // the range unless it is the last.
          if(*iter == leaf2)
            {
            (*iter)->setSecond(second);
            doAdd = false;
            }
          else
            {
            first = (*iter)->getFirst();
            this->removeNode(*iter);
            delete *iter;
            }
          }
        else if(first <= (*iter)->getSecond())
          {
          if(first == (*iter)->getFirst())
            {
            if(*iter == leaf2)
              {
              // Move the leaf to the new range.
              (*iter)->setFirst((*iter)->getSecond() + 1);
              (*iter)->setSecond(second);
              doAdd = false;
              }
            else
              {
              // Adjust the range and remove the leaf.
              first = (*iter)->getSecond() + 1;
              this->removeNode(*iter);
              delete *iter;
              }
            }
          else
            {
            // Two ranges will be created. Move the leaf to the left range.
            int temp = first;
            first = (*iter)->getSecond() + 1;
            if(temp < (*iter)->getFirst())
              {
              (*iter)->setSecond((*iter)->getFirst() - 1);
              (*iter)->setFirst(temp);
              }
            else
              {
              (*iter)->setSecond(temp - 1);
              }
            }
          }
        // If first > (*iter)->getSecond() + 1, the range does not
        // intersect the leaf.
        }
      else if(second == (*iter)->getSecond())
        {
        if(first == (*iter)->getFirst())
          {
          // Remove the leaf.
          this->removeNode(*iter);
          delete *iter;
          }
        else if(first < (*iter)->getFirst())
          {
          // Move the leaf range.
          (*iter)->setSecond((*iter)->getFirst() - 1);
          (*iter)->setFirst(first);
          }
        else
          {
          // Shrink the leaf range.
          (*iter)->setSecond(first - 1);
          }

        doAdd = false;
        }
      else if(first == (*iter)->getFirst())
        {
        // Shrink the leaf range.
        (*iter)->setFirst(second + 1);
        doAdd = false;
        }
      else
        {
        // Two ranges will be created. Move the leaf to the left range.
        int temp = first;
        first = second + 1;
        second = (*iter)->getSecond();
        if(temp < (*iter)->getFirst())
          {
          (*iter)->setSecond((*iter)->getFirst() - 1);
          (*iter)->setFirst(temp);
          }
        else
          {
          (*iter)->setSecond(temp - 1);
          }
        }
      }

    if(doAdd)
      {
      // Add the range after the last leaf.
      this->insertNode(leaf2, new vtkQtChartIndexRange(first, second),
          false);
      }
    }
  else
    {
    // If there is no closest, the tree is empty.
    this->Root = new vtkQtChartIndexRange(first, second);
    }

  return true;
}

bool vtkQtChartIndexRangeList::xorRanges(
    const vtkQtChartIndexRangeList &ranges)
{
  if(ranges.isEmpty())
    {
    return false;
    }

  // Xor each of the ranges with the tree.
  vtkQtChartIndexRange *range = ranges.getFirst();
  while(range)
    {
    this->xorRange(range->getFirst(), range->getSecond());
    range = ranges.getNext(range);
    }

  return true;
}

void vtkQtChartIndexRangeList::limitRange(int minimum, int maximum)
{
  if(this->Root && minimum > this->Root->getFirst())
    {
    this->subtractRange(this->Root->getFirst(), minimum - 1);
    }

  if(this->Root && maximum < this->Root->getSecond())
    {
    this->subtractRange(maximum + 1, this->Root->getSecond());
    }
}

bool vtkQtChartIndexRangeList::offsetRanges(int start, int offset)
{
  bool changed = false;
  vtkQtChartIndexRange *range = this->findClosest(start);
  while(range)
    {
    if(range->getFirst() >= start)
      {
      range->setFirst(range->getFirst() + offset);
      range->setSecond(range->getSecond() + offset);
      changed = true;
      }
    else if(range->getSecond() >= start)
      {
      range->setSecond(range->getSecond() + offset);
      changed = true;
      }

    range = this->getNext(range);
    }

  return changed;
}

vtkQtChartIndexRange *vtkQtChartIndexRangeList::findNode(int value,
    bool left) const
{
  vtkQtChartIndexRange *range = this->findClosest(value);
  if(range)
    {
    if(left)
      {
      if(value < range->getFirst())
        {
        vtkQtChartIndexRange *previous = this->getPrevious(range);
        if(previous)
          {
          range = previous;
          }
        }
      }
    else
      {
      if(value > range->getSecond())
        {
        vtkQtChartIndexRange *next = this->getNext(range);
        if(next)
          {
          range = next;
          }
        }
      }
    }

  return range;
}

void vtkQtChartIndexRangeList::rotateLeft(vtkQtChartIndexRange *node)
{
  vtkQtChartIndexRange *right = node->getRight();
  vtkQtChartIndexRange *parent = node->getParent();
  right->setParent(parent);
  node->setParent(right);
  if(!parent)
    {
    // Replace the pointer to the root if the right node becomes the
    // new root.
    this->Root = right;
    }
  else if(node == parent->getLeft())
    {
    parent->setLeft(right);
    }
  else
    {
    parent->setRight(right);
    }

  vtkQtChartIndexRange *left = right->getLeft();
  node->setRight(left);
  right->setLeft(node);
  left->setParent(node);

  // Update the node ranges.
  right->setFirst(node->getFirst());
  node->setSecond(left->getSecond());
}

void vtkQtChartIndexRangeList::rotateRight(vtkQtChartIndexRange *node)
{
  vtkQtChartIndexRange *left = node->getLeft();
  vtkQtChartIndexRange *parent = node->getParent();
  left->setParent(parent);
  node->setParent(left);
  if(!parent)
    {
    // Replace the pointer to the root if the left node becomes the
    // new root.
    this->Root = left;
    }
  else if(node == parent->getLeft())
    {
    parent->setLeft(left);
    }
  else
    {
    parent->setRight(left);
    }

  vtkQtChartIndexRange *right = left->getRight();
  node->setLeft(right);
  left->setRight(node);
  right->setParent(node);

  // Update the node ranges.
  left->setSecond(node->getSecond());
  node->setFirst(right->getFirst());
}

void vtkQtChartIndexRangeList::insertNode(vtkQtChartIndexRange *current,
    vtkQtChartIndexRange *node, bool left)
{
  // Create a new node to take the place of the current one.
  vtkQtChartIndexRange *parent = new vtkQtChartIndexRange();

  // Remove the current node from the tree.
  vtkQtChartIndexRange *grandpa = current->getParent();
  parent->setParent(grandpa);
  if(!grandpa)
    {
    this->Root = parent;
    }
  else if(current == grandpa->getLeft())
    {
    grandpa->setLeft(parent);
    }
  else
    {
    grandpa->setRight(parent);
    }

  // Add the leaf nodes to the new parent.
  current->setParent(parent);
  node->setParent(parent);
  if(left)
    {
    parent->setLeft(node);
    parent->setRight(current);
    parent->setFirst(node->getFirst());
    parent->setSecond(current->getSecond());
    }
  else
    {
    parent->setLeft(current);
    parent->setRight(node);
    parent->setFirst(current->getFirst());
    parent->setSecond(node->getSecond());
    }

  // Rebalance at the new parent node.
  node = parent;
  while(node)
    {
    parent = node->getParent();
    if(parent == 0)
      {
      // The node becomes the new root and must be black.
      node->setBlack(true);
      }
    else if(!parent->isBlack())
      {
      grandpa = parent->getParent();
      bool isParentLeft = parent == grandpa->getLeft();
      vtkQtChartIndexRange *uncle = grandpa->getLeft();
      if(isParentLeft)
        {
        uncle = grandpa->getRight();
        }

      if(uncle && !uncle->isBlack())
        {
        parent->setBlack(true);
        uncle->setBlack(true);
        grandpa->setBlack(false);

        // Rebalance the grandparent.
        node = grandpa;
        continue;
        }
      else
        {
        if(node == parent->getRight() && isParentLeft)
          {
          this->rotateLeft(parent);

          // Fix the node and parent pointers for the rotation.
          node = parent;
          parent = node->getParent();
          }
        else if(node == parent->getLeft() && !isParentLeft)
          {
          this->rotateRight(parent);

          // Fix the node and parent pointers for the rotation.
          node = parent;
          parent = node->getParent();
          }

        // Set the parent and grandparent colors.
        parent->setBlack(true);
        grandpa->setBlack(false);
        if(node == parent->getLeft() && isParentLeft)
          {
          this->rotateRight(grandpa);
          }
        else // node == parent->getRight() && !isParentLeft
          {
          this->rotateLeft(grandpa);
          }
        }
      }

    // All cases but one exit the loop.
    break;
    }
}

void vtkQtChartIndexRangeList::removeNode(vtkQtChartIndexRange *node)
{
  // Remove the node from its parent.
  vtkQtChartIndexRange *parent = node->getParent();
  node->setParent(0);
  if(!parent)
    {
    // The tree becomes empty when the last leaf is removed.
    this->Root = 0;
    return;
    }

  // Get the node's sibling.
  vtkQtChartIndexRange *child = 0;
  if(node == parent->getLeft())
    {
    child = parent->getRight();
    }
  else
    {
    child = parent->getLeft();
    }

  // Clear the parent node's child pointers for deletion.
  parent->setLeft(0);
  parent->setRight(0);

  // Put the other child in the place of the parent node. From here on,
  // the original node pointer is ignored and replaced with the parent.
  node = parent;
  parent = node->getParent();
  node->setParent(0);
  child->setParent(parent);
  if(parent)
    {
    if(node == parent->getLeft())
      {
      parent->setLeft(child);
      parent->setFirst(child->getFirst());
      }
    else
      {
      parent->setRight(child);
      parent->setSecond(child->getSecond());
      }
    }
  else
    {
    this->Root = child;
    }

  // Rebalance the tree if necessary.
  if(node->isBlack())
    {
    if(child->isBlack())
      {
      vtkQtChartIndexRange *sibling = 0;
      while(parent != 0)
        {
        // If the sibling is red, rotate it to get a black sibling.
        bool isLeft = child == parent->getLeft();
        sibling = isLeft ? parent->getRight() : parent->getLeft();
        if(!sibling->isBlack())
          {
          // Switch the color of sibling and parent.
          parent->setBlack(false);
          sibling->setBlack(true);
          if(isLeft)
            {
            this->rotateLeft(parent);
            }
          else
            {
            this->rotateRight(parent);
            }

          // Get the new sibling  after the rotation.
          sibling = isLeft ? parent->getRight() : parent->getLeft();
          }

        // Check the sibling's children's colors.
        vtkQtChartIndexRange *left = sibling->getLeft();
        vtkQtChartIndexRange *right = sibling->getRight();
        if(sibling->isBlack() && left->isBlack() && right->isBlack())
          {
          sibling->setBlack(false);
          if(parent->isBlack())
            {
            // If all the surrounding nodes are black, rebalance the parent.
            child = parent;
            parent = child->getParent();
            continue;
            }
          else
            {
            parent->setBlack(true);
            }
          }
        else
          {
          if(sibling->isBlack())
            {
            if(isLeft && right->isBlack() && !left->isBlack())
              {
              sibling->setBlack(false);
              left->setBlack(true);
              this->rotateRight(sibling);

              // Update the node pointers for the rotation.
              right = sibling;
              sibling = left;
              left = sibling->getLeft();
              }
            else if(!isLeft && left->isBlack() && !right->isBlack())
              {
              sibling->setBlack(false);
              right->setBlack(true);
              this->rotateLeft(sibling);

              // Update the node pointers for the rotation.
              left = sibling;
              sibling = right;
              right = sibling->getRight();
              }
            }

          // Finally, rotate the parent to finish rebalancing the tree.
          sibling->setBlack(parent->isBlack());
          parent->setBlack(true);
          if(isLeft)
            {
            right->setBlack(true);
            this->rotateLeft(parent);
            }
          else
            {
            left->setBlack(true);
            this->rotateRight(parent);
            }
          }

        // All but one case exits the loop.
        break;
        }
      }
    else
      {
      // Set the child color to black and the tree is balanced.
      child->setBlack(true);
      }
    }

  delete node; // Note: this is the parent of the node passed in.
}

int vtkQtChartIndexRangeList::removeBetween(vtkQtChartIndexRange *left,
    vtkQtChartIndexRange *right)
{
  QList<vtkQtChartIndexRange *> list;
  vtkQtChartIndexRange *node = this->getNext(left);
  while(node && node != right)
    {
    list.append(node);
    node = this->getNext(node);
    }

  QList<vtkQtChartIndexRange *>::Iterator iter = list.begin();
  for( ; iter != list.end(); ++iter)
    {
    this->removeNode(*iter);
    delete *iter;
    }

  return list.size();
}

vtkQtChartIndexRange *vtkQtChartIndexRangeList::getNextNode(
    vtkQtChartIndexRange *node) const
{
  if(node)
    {
    vtkQtChartIndexRange *child = node->getLeft();
    if(child)
      {
      return child;
      }

    // Search the parent chain for the next sibling.
    vtkQtChartIndexRange *parent = node->getParent();
    while(parent)
      {
      if(node == parent->getLeft())
        {
        return parent->getRight();
        }

      node = parent;
      parent = node->getParent();
      }
    }

  return 0;
}

vtkQtChartIndexRange *vtkQtChartIndexRangeList::getPreviousNode(
    vtkQtChartIndexRange *node) const
{
  if(node)
    {
    vtkQtChartIndexRange *parent = node->getParent();
    if(parent)
      {
      vtkQtChartIndexRange *left = parent->getLeft();
      if(left && left != node)
        {
        return this->getLastNode(left);
        }
      else
        {
        return parent;
        }
      }
    }

  return 0;
}

vtkQtChartIndexRange *vtkQtChartIndexRangeList::getFirstNode(
    vtkQtChartIndexRange *node) const
{
  while(node && node->getLeft())
    {
    node = node->getLeft();
    }

  return node;
}

vtkQtChartIndexRange *vtkQtChartIndexRangeList::getLastNode(
    vtkQtChartIndexRange *node) const
{
  while(node && node->getRight())
    {
    node = node->getRight();
    }

  return node;
}


