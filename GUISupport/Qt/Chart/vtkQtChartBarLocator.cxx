/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtChartBarLocator.cxx

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

/// \file vtkQtChartBarLocator.cxx
/// \date October 1, 2008

#ifdef _MSC_VER
// Disable warnings that Qt headers give.
#pragma warning(disable:4127)
#endif

#include "vtkQtChartBarLocator.h"

#include "vtkQtChartBar.h"

#include <QMutableLinkedListIterator>
#include <QPointF>
#include <QRectF>


//-----------------------------------------------------------------------------
vtkQtChartBarLocatorNode::vtkQtChartBarLocatorNode(vtkQtChartBar *element)
{
  this->Bounds = new QRectF();
  this->Element = 0;
  this->Parent = 0;
  this->First = 0;
  this->Second = 0;

  this->setElement(element);
}

vtkQtChartBarLocatorNode::~vtkQtChartBarLocatorNode()
{
  delete this->Bounds;

  // Delete the child nodes.
  if(this->First)
    {
    delete this->First;
    }

  if(this->Second)
    {
    delete this->Second;
    }
}

bool vtkQtChartBarLocatorNode::contains(const QPointF &point) const
{
  return this->Bounds->contains(point);
}

bool vtkQtChartBarLocatorNode::intersects(const QRectF &area) const
{
  // QRectF's intersects method misses when width or height is zero.
  return qMax<float>(this->Bounds->left(), area.left()) <= 
      qMin<float>(this->Bounds->right(), area.right()) &&
      qMax<float>(this->Bounds->top(), area.top()) <= 
      qMin<float>(this->Bounds->bottom(), area.bottom());
}

void vtkQtChartBarLocatorNode::setBounds(const QRectF &bounds)
{
  *this->Bounds = bounds;
}

void vtkQtChartBarLocatorNode::setElement(vtkQtChartBar *element)
{
  if(this->Element != element)
    {
    this->Element = element;
    if(this->Element)
      {
      this->Element->getBounds(*this->Bounds);
      }
    }
}

void vtkQtChartBarLocatorNode::updateBounds()
{
  if(this->Element)
    {
    this->Element->getBounds(*this->Bounds);
    }
  else if(this->First)
    {
    if(this->Second)
      {
      *this->Bounds = this->First->getBounds().united(
          this->Second->getBounds());
      }
    else
      {
      *this->Bounds = this->First->getBounds();
      }
    }
  else if(this->Second)
    {
    *this->Bounds = this->Second->getBounds();
    }
  else
    {
    this->Bounds->setRect(0.0, 0.0, 0.0, 0.0);
    }
}


//-----------------------------------------------------------------------------
vtkQtChartBarLocator::vtkQtChartBarLocator()
  : Items()
{
  this->Root = 0;
}

vtkQtChartBarLocator::~vtkQtChartBarLocator()
{
  // Delete the root node.
  if(this->Root)
    {
    delete this->Root;
    }
}

void vtkQtChartBarLocator::clear()
{
  if(this->Root)
    {
    delete this->Root;
    this->Root = 0;

    this->Items.clear();
    }
}

void vtkQtChartBarLocator::build(const QList<vtkQtChartBar *> &list)
{
  // Clean up the current tree.
  this->clear();

  // Create a copy of the list with bar tree nodes.
  QLinkedList<vtkQtChartBarLocatorNode *> temp;
  QList<vtkQtChartBar *>::ConstIterator iter = list.begin();
  for( ; iter != list.end(); ++iter)
    {
    vtkQtChartBarLocatorNode *leaf = new vtkQtChartBarLocatorNode(*iter);
    this->Items.append(leaf);
    temp.append(leaf);
    }

  // Build the tree from the temporary linked list.
  this->buildTree(temp);

  if(temp.size() > 0)
    {
    // Save a pointer to the root.
    this->Root = temp.first();
    }
}

void vtkQtChartBarLocator::update()
{
  vtkQtChartBarLocatorNode *node = this->getLast();
  while(node)
    {
    node->updateBounds();
    node = this->getPrevious(node);
    }
}

vtkQtChartBar *vtkQtChartBarLocator::getItemAt(const QPointF &point) const
{
  // Make sure the point is within the overall width.
  if(this->Root && this->Root->contains(point))
    {
    float px = point.x();
    float py = point.y();
    vtkQtChartBarLocatorNode *node = this->Root;
    vtkQtChartBarLocatorNode *first = this->Root->getFirst();
    vtkQtChartBarLocatorNode *second = this->Root->getSecond();
    while(first && second)
      {
      // Check the right side first. If bars overlap in a bar chart,
      // the right one is on top.
      if(px >= second->getBounds().left() && py >= second->getBounds().top() &&
          py <= second->getBounds().bottom())
        {
        node = second;
        }
      else if(px <= first->getBounds().right() &&
          py >= first->getBounds().top() && py <= first->getBounds().bottom())
        {
        node = first;
        }
      else
        {
        return 0;
        }

      // Get the new node's children.
      first = node->getFirst();
      second = node->getSecond();
      }

    return node->getElement();
    }

  return 0;
}

QList<vtkQtChartBar *> vtkQtChartBarLocator::getItemsIn(
    const QRectF &area) const
{
  // Make sure the rectangle intersects the overall bounds.
  QList<vtkQtChartBar *> items;
  if(this->Root && this->Root->intersects(area))
    {
    // First, find the left bounds of the search area.
    float left = area.left();
    vtkQtChartBarLocatorNode *node = this->Root;
    vtkQtChartBarLocatorNode *first = this->Root->getFirst();
    vtkQtChartBarLocatorNode *second = this->Root->getSecond();
    int leftId = 0;
    if(left > this->Root->getBounds().left())
      {
      while(first && second)
        {
        if(left <= first->getBounds().right())
          {
          node = first;
          }
        else if(left >= second->getBounds().left())
          {
          node = second;
          }
        else
          {
          // Get the bar to the right of the point.
          node = second;
          first = node->getFirst();
          while(first)
            {
            node = first;
            first = node->getFirst();
            }

          break;
          }

        // Get the new node's children.
        first = node->getFirst();
        second = node->getSecond();
        }

      leftId = this->Items.indexOf(node);
      }

    // Next, find the right bounds of the search area.
    float right = area.right();
    int rightId = this->Items.size() - 1;
    if(right < this->Root->getBounds().right())
      {
      node = this->Root;
      first = this->Root->getFirst();
      second = this->Root->getSecond();
      while(first && second)
        {
        if(right >= second->getBounds().left())
          {
          node = second;
          }
        else if(right <= first->getBounds().right())
          {
          node = first;
          }
        else
          {
          // Get the bar to the left of the point.
          node = first;
          second = node->getSecond();
          while(second)
            {
            node = second;
            second = node->getSecond();
            }

          break;
          }

        // Get the new node's children.
        first = node->getFirst();
        second = node->getSecond();
        }

      rightId = this->Items.indexOf(node);
      }

    if(leftId <= rightId)
      {
      // If the search range is valid, determine which nodes are in the
      // search area height.
      float top = area.top();
      float bottom = area.bottom();
      for(int i = leftId; i <= rightId; i++)
        {
        node = this->Items[i];
        if(node->getBounds().top() <= bottom &&
            node->getBounds().bottom() >= top)
          {
          items.append(node->getElement());
          }
        }
      }
    }

  return items;
}

vtkQtChartBarLocatorNode *vtkQtChartBarLocator::getLast()
{
  vtkQtChartBarLocatorNode *last = 0;
  vtkQtChartBarLocatorNode *node = this->Root;
  while(node && node->hasChildren())
    {
    last = node->getSecond();
    if(!last)
      {
      last = node->getFirst();
      }

    node = last;
    }

  return node;
}

vtkQtChartBarLocatorNode *vtkQtChartBarLocator::getPrevious(
    vtkQtChartBarLocatorNode *node)
{
  if(node)
    {
    vtkQtChartBarLocatorNode *parent = node->getParent();
    if(parent)
      {
      if(node == parent->getFirst() ||
          (!parent->getFirst() && node == parent->getSecond()))
        {
        return parent;
        }
      else
        {
        node = parent->getFirst();
        vtkQtChartBarLocatorNode *last = 0;
        while(node && node->hasChildren())
          {
          last = node->getSecond();
          if(!last)
            {
            last = node->getFirst();
            }

          node = last;
          }

        return node;
        }
      }
    }

  return 0;
}

void vtkQtChartBarLocator::buildTree(
    QLinkedList<vtkQtChartBarLocatorNode *> &list) const
{
  // Combine elements in the list until there is only one.
  for(int i = 0; list.size() > 1; i++)
    {
    int count = list.size();
    bool odd = (i % 2) == 0;
    QMutableLinkedListIterator<vtkQtChartBarLocatorNode *> iter(list);
    if(odd)
      {
      iter.toBack();
      }

    vtkQtChartBarLocatorNode *parent = 0;
    vtkQtChartBarLocatorNode *first = 0;
    vtkQtChartBarLocatorNode *second = 0;
    while(count > 1)
      {
      if((odd && !iter.hasPrevious()) || (!odd && !iter.hasNext()))
        {
        break;
        }

      // Get the next two objects from the list.
      count -= 2;
      if(odd)
        {
        second = iter.previous();
        iter.remove();
        first = iter.previous();
        iter.remove();
        }
      else
        {
        first = iter.next();
        iter.remove();
        second = iter.next();
        iter.remove();
        }

      // Combine the elements in a new parent element.
      parent = new vtkQtChartBarLocatorNode();
      parent->setFirst(first);
      parent->setSecond(second);
      first->setParent(parent);
      second->setParent(parent);
      parent->updateBounds();

      // Insert the new parent into the list.
      iter.insert(parent);
      if(odd)
        {
        iter.previous();
        }
      }
    }
}


