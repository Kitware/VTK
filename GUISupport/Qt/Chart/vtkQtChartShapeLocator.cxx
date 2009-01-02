/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtChartShapeLocator.cxx

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

/// \file vtkQtChartShapeLocator.cxx
/// \date October 15, 2008

#ifdef _MSC_VER
// Disable warnings that Qt headers give.
#pragma warning(disable:4127)
#endif

#include "vtkQtChartShapeLocator.h"

#include "vtkQtChartShape.h"

#include <QMutableLinkedListIterator>
#include <QPair>
#include <QPointF>
#include <QRectF>

#include "math.h"


//-----------------------------------------------------------------------------
vtkQtChartShapeLocatorNode::vtkQtChartShapeLocatorNode(vtkQtChartShape *element)
  : Nodes()
{
  this->Bounds = new QRectF();
  this->Element = 0;
  this->Parent = 0;

  this->setElement(element);
}

vtkQtChartShapeLocatorNode::~vtkQtChartShapeLocatorNode()
{
  delete this->Bounds;
  QList<vtkQtChartShapeLocatorNode *>::Iterator iter = this->Nodes.begin();
  for( ; iter != this->Nodes.end(); ++iter)
    {
    delete *iter;
    }
}

bool vtkQtChartShapeLocatorNode::contains(const QPointF &point) const
{
  if(this->Element)
    {
    return this->Element->contains(point);
    }
  else
    {
    return this->Bounds->contains(point);
    }
}

bool vtkQtChartShapeLocatorNode::intersects(const QRectF &area) const
{
  if(this->Element)
    {
    return this->Element->intersects(area);
    }
  else
    {
    // QRectF's intersects method misses when width or height is zero.
    return qMax<float>(this->Bounds->left(), area.left()) <= 
        qMin<float>(this->Bounds->right(), area.right()) &&
        qMax<float>(this->Bounds->top(), area.top()) <= 
        qMin<float>(this->Bounds->bottom(), area.bottom());
    }
}

void vtkQtChartShapeLocatorNode::setBounds(const QRectF &bounds)
{
  *this->Bounds = bounds;
}

void vtkQtChartShapeLocatorNode::setElement(vtkQtChartShape *element)
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

void vtkQtChartShapeLocatorNode::updateBounds()
{
  if(this->Element)
    {
    this->Element->getBounds(*this->Bounds);
    }
  else
    {
    bool first = true;
    QList<vtkQtChartShapeLocatorNode *>::Iterator iter = this->Nodes.begin();
    for( ; iter != this->Nodes.end(); ++iter)
      {
      if(first)
        {
        *this->Bounds = (*iter)->getBounds();
        first = false;
        }
      else
        {
        *this->Bounds = this->Bounds->united((*iter)->getBounds());
        }
      }
    }
}


//-----------------------------------------------------------------------------
vtkQtChartShapeLocator::vtkQtChartShapeLocator()
{
  this->Root = 0;
}

vtkQtChartShapeLocator::~vtkQtChartShapeLocator()
{
  // Delete the root node.
  if(this->Root)
    {
    delete this->Root;
    }
}

void vtkQtChartShapeLocator::clear()
{
  if(this->Root)
    {
    delete this->Root;
    this->Root = 0;
    }
}

void vtkQtChartShapeLocator::build(
    const QList<QList<vtkQtChartShape *> > &table)
{
  // Clean up the current tree.
  this->clear();
  if(table.size() == 0)
    {
    return;
    }

  // Create a linked list copy of the table to build the tree.
  QLinkedList<QLinkedList<vtkQtChartShapeLocatorNode *> > nodeTable;
  QList<QList<vtkQtChartShape *> >::ConstIterator iter = table.begin();
  for( ; iter != table.end(); ++iter)
    {
    nodeTable.append(QLinkedList<vtkQtChartShapeLocatorNode *>());
    QList<vtkQtChartShape *>::ConstIterator jter = iter->begin();
    for( ; jter != iter->end(); ++jter)
      {
      nodeTable.last().append(new vtkQtChartShapeLocatorNode(*jter));
      }
    }

  this->build(nodeTable);
}

void vtkQtChartShapeLocator::build(const QList<vtkQtChartShape *> &list)
{
  // The list should already be sorted in x. Divide the list into
  // equal portions.
  QList<QList<vtkQtChartShape *> > table;
  int length = (int)(sqrt((double)(list.size() * list.size())) + 0.5);
  QList<vtkQtChartShape *>::ConstIterator iter = list.begin();
  while(iter != list.end())
    {
    table.append(QList<vtkQtChartShape *>());
    for(int i = 0; i < length && iter != list.end(); ++iter, ++i)
      {
      table.last().append(*iter);
      }
    }
  
  // Then, sort each portion in y.
  QList<QList<vtkQtChartShape *> >::Iterator jter = table.begin();
  for( ; jter != table.end(); ++jter)
    {
    vtkQtChartShapeLocator::sort(*jter);
    }

  // Finally, build the tree from the table.
  this->build(table);
}

void vtkQtChartShapeLocator::update()
{
  vtkQtChartShapeLocatorNode *node = this->getLast();
  while(node)
    {
    node->updateBounds();
    node = this->getPrevious(node);
    }
}

QList<vtkQtChartShape *> vtkQtChartShapeLocator::getItemsAt(
    const QPointF &point) const
{
  QList<vtkQtChartShape *> shapes;
  if(this->Root && this->Root->contains(point))
    {
    QLinkedList<vtkQtChartShapeLocatorNode *> checkList;
    vtkQtChartShape *shape = this->Root->getElement();
    if(shape)
      {
      shapes.append(shape);
      }
    else
      {
      checkList.append(this->Root);
      }

    while(checkList.size() > 0)
      {
      QMutableLinkedListIterator<vtkQtChartShapeLocatorNode *> iter(checkList);
      while(iter.hasNext())
        {
        // Check each of the child nodes.
        vtkQtChartShapeLocatorNode *parent = iter.next();
        iter.remove();
        QList<vtkQtChartShapeLocatorNode *>::ConstIterator jter =
            parent->getNodes().begin();
        for( ; jter != parent->getNodes().end(); ++jter)
          {
          if((*jter)->contains(point))
            {
            // If the node is a leaf, add the shape to the list of hits.
            // Otherwise, add the child node to the list to be searched.
            shape = (*jter)->getElement();
            if(shape)
              {
              shapes.append(shape);
              }
            else
              {
              iter.insert((*jter));
              }
            }
          }
        }
      }
    }

  return shapes;
}

QList<vtkQtChartShape *> vtkQtChartShapeLocator::getItemsIn(
    const QRectF &area) const
{
  QList<vtkQtChartShape *> shapes;
  if(this->Root && this->Root->intersects(area))
    {
    QLinkedList<vtkQtChartShapeLocatorNode *> checkList;
    vtkQtChartShape *shape = this->Root->getElement();
    if(shape)
      {
      shapes.append(shape);
      }
    else
      {
      checkList.append(this->Root);
      }

    while(checkList.size() > 0)
      {
      QMutableLinkedListIterator<vtkQtChartShapeLocatorNode *> iter(checkList);
      while(iter.hasNext())
        {
        // Check each of the child nodes.
        vtkQtChartShapeLocatorNode *parent = iter.next();
        iter.remove();
        QList<vtkQtChartShapeLocatorNode *>::ConstIterator jter =
            parent->getNodes().begin();
        for( ; jter != parent->getNodes().end(); ++jter)
          {
          if((*jter)->intersects(area))
            {
            // If the node is a leaf, add the shape to the list of hits.
            // Otherwise, add the child node to the list to be searched.
            shape = (*jter)->getElement();
            if(shape)
              {
              shapes.append(shape);
              }
            else
              {
              iter.insert((*jter));
              }
            }
          }
        }
      }
    }

  return shapes;
}

vtkQtChartShapeLocatorNode *vtkQtChartShapeLocator::getLast()
{
  vtkQtChartShapeLocatorNode *node = this->Root;
  while(node && node->hasChildren())
    {
    node = node->getNodes().last();
    }

  return node;
}

vtkQtChartShapeLocatorNode *vtkQtChartShapeLocator::getPrevious(
    vtkQtChartShapeLocatorNode *node)
{
  if(node)
    {
    vtkQtChartShapeLocatorNode *parent = node->getParent();
    if(parent)
      {
      int index = parent->getNodes().indexOf(node);
      if(index == 0)
        {
        return parent;
        }
      else if(index > 0)
        {
        node = parent->getNodes().at(index - 1);
        while(node && node->hasChildren())
          {
          node = node->getNodes().last();
          }

        return node;
        }
      }
    }

  return 0;
}

void vtkQtChartShapeLocator::build(
    QLinkedList<QLinkedList<vtkQtChartShapeLocatorNode *> > &table)
{
  // Combine elements in the table until there is only one.
  for(int i = 0; table.size() > 0; i++)
    {
    if(table.size() == 1 && table.first().size() == 1)
      {
      break;
      }

    bool odd = (i % 2) == 0;
    QMutableLinkedListIterator<QLinkedList<vtkQtChartShapeLocatorNode *> > iter(
        table);
    if(odd)
      {
      iter.toBack();
      }

    while((odd && iter.hasPrevious()) || (!odd && iter.hasNext()))
      {
      // Get the next two lists from the table.
      QLinkedList<vtkQtChartShapeLocatorNode *> first;
      QLinkedList<vtkQtChartShapeLocatorNode *> second;
      if(odd)
        {
        second = iter.previous();
        iter.remove();
        if(iter.hasPrevious())
          {
          first = iter.previous();
          iter.remove();
          }
        }
      else
        {
        first = iter.next();
        iter.remove();
        if(iter.hasNext())
          {
          second = iter.next();
          iter.remove();
          }
        }

      // Combine the list elements into the left list.
      int firstCount = first.size();
      int secondCount = second.size();
      QMutableLinkedListIterator<vtkQtChartShapeLocatorNode *> jter(first);
      QMutableLinkedListIterator<vtkQtChartShapeLocatorNode *> kter(second);
      if(odd)
        {
        jter.toBack();
        kter.toBack();
        }

      vtkQtChartShapeLocatorNode *node = 0;
      while(firstCount > 1 || secondCount > 0)
        {
        if(firstCount == 0 && secondCount == 1)
          {
          // Move the remaining node from the second list to the first.
          node = odd ? kter.previous() : kter.next();
          kter.remove();
          jter.insert(node);
          break;
          }

        // Create a new parent element for the nodes.
        vtkQtChartShapeLocatorNode *parent = new vtkQtChartShapeLocatorNode();

        // Get the next four elements from the two lists.
        if(odd)
          {
          if(jter.hasPrevious())
            {
            node = jter.previous(); // Bottom-left
            jter.remove();
            node->setParent(parent);
            parent->getNodes().insert(0, node);
            firstCount--;
            }

          if(kter.hasPrevious())
            {
            node = kter.previous(); // Bottom-right
            kter.remove();
            node->setParent(parent);
            parent->getNodes().insert(0, node);
            secondCount--;
            }

          if(kter.hasPrevious())
            {
            node = kter.previous(); // Top-right
            kter.remove();
            node->setParent(parent);
            parent->getNodes().insert(0, node);
            secondCount--;
            }

          if(jter.hasPrevious())
            {
            node = jter.previous(); // Top-left
            jter.remove();
            node->setParent(parent);
            parent->getNodes().insert(0, node);
            firstCount--;
            }
          }
        else
          {
          if(jter.hasNext())
            {
            node = jter.next(); // Top-left
            jter.remove();
            node->setParent(parent);
            parent->getNodes().append(node);
            firstCount--;
            }

          if(kter.hasNext())
            {
            node = kter.next(); // Top-right
            kter.remove();
            node->setParent(parent);
            parent->getNodes().append(node);
            secondCount--;
            }

          if(kter.hasNext())
            {
            node = kter.next(); // Bottom-right
            kter.remove();
            node->setParent(parent);
            parent->getNodes().append(node);
            secondCount--;
            }

          if(jter.hasNext())
            {
            node = jter.next(); // Bottom-left
            jter.remove();
            node->setParent(parent);
            parent->getNodes().append(node);
            firstCount--;
            }
          }

        // Update the bounds and add the new node to the first list.
        parent->updateBounds();
        jter.insert(parent);
        if(odd)
          {
          jter.previous();
          }
        }

      // Add the new list back into the table.
      iter.insert(first);
      if(odd)
        {
        iter.previous();
        }
      }
    }

  // Save the root object pointer.
  if(table.size() > 0 && table.first().size() > 0)
    {
    this->Root = table.first().first();
    }
}

void vtkQtChartShapeLocator::sort(QList<vtkQtChartShape *> &list)
{
  if(list.size() < 2)
    {
    return;
    }

  // Use a boundary list to avoid recursive calls.
  QRectF area;
  float first = 0.0, last = 0.0;
  vtkQtChartShape *temp = 0;
  QLinkedList<QPair<int, int> > bounds;
  bounds.append(QPair<int, int>(0, list.size() - 1));
  while(bounds.size() > 0)
    {
    QMutableLinkedListIterator<QPair<int, int> > jter(bounds);
    while(jter.hasNext())
      {
      QPair<int, int> range = jter.next();
      jter.remove();
      int length = range.second - range.first + 1;
      if(length == 2)
        {
        // Swap the shapes if necessary.
        list[range.first]->getBounds(area);
        first = area.center().y();
        list[range.second]->getBounds(area);
        last = area.center().y();
        if(last < first)
          {
          temp = list[range.second];
          list[range.second] = list[range.first];
          list[range.first] = temp;
          }
        }
      else
        {
        // Find the pivot point from the first, last and middle point.
        int pivot = range.first + length / 2;
        list[range.first]->getBounds(area);
        first = area.center().y();
        list[pivot]->getBounds(area);
        float middle = area.center().y();
        list[range.second]->getBounds(area);
        last = area.center().y();
        if(first > middle)
          {
          if(!(middle > last))
            {
            if(first > last)
              {
              pivot = range.second;
              middle = last;
              }
            else
              {
              pivot = range.first;
              middle = first;
              }
            }
          }
        else if(middle > last)
          {
          if(first > last)
            {
            pivot = range.first;
            middle = first;
            }
          else
            {
            pivot = range.second;
            middle = last;
            }
          }

        // Swap the pivot and last point.
        if(pivot != range.second)
          {
          temp = list[range.second];
          list[range.second] = list[pivot];
          list[pivot] = temp;
          }

        // Partition the remaining points.
        pivot = range.first;
        for(int i = range.first; i < range.second; i++)
          {
          list[i]->getBounds(area);
          if(area.center().y() <= middle)
            {
            // Swap the points if necessary.
            if(pivot != i)
              {
              temp = list[i];
              list[i] = list[pivot];
              list[pivot] = temp;
              }

            pivot++;
            }
          }

        // Move the last point to the partition.
        if(pivot != range.second)
          {
          temp = list[range.second];
          list[range.second] = list[pivot];
          list[pivot] = temp;
          }

        // Add ranges to the bounds list for the two partitions.
        length = pivot - range.first;
        if(length > 1)
          {
          jter.insert(QPair<int, int>(range.first, pivot - 1));
          }

        length = range.second - pivot;
        if(length > 1)
          {
          jter.insert(QPair<int, int>(pivot + 1, range.second));
          }
        }
      }
    }
}


