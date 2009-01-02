/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtChartShapeLocator.h

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

/// \file vtkQtChartShapeLocator.h
/// \date October 15, 2008

#ifndef _vtkQtChartShapeLocator_h
#define _vtkQtChartShapeLocator_h

#include "vtkQtChartExport.h"
#include <QLinkedList> // Needed for parameter.
#include <QList> // Needed for parameter.

class vtkQtChartShape;
class QPointF;
class QRectF;


class VTKQTCHART_EXPORT vtkQtChartShapeLocatorNode
{
public:
  vtkQtChartShapeLocatorNode(vtkQtChartShape *element=0);
  ~vtkQtChartShapeLocatorNode();

  bool contains(const QPointF &point) const;
  bool intersects(const QRectF &area) const;

  const QRectF &getBounds() const {return *this->Bounds;}
  void setBounds(const QRectF &bounds);

  vtkQtChartShape *getElement() const {return this->Element;}
  void setElement(vtkQtChartShape *element);

  vtkQtChartShapeLocatorNode *getParent() const {return this->Parent;}
  void setParent(vtkQtChartShapeLocatorNode *parent) {this->Parent = parent;}

  bool hasChildren() const {return this->Nodes.size() > 0;}
  const QList<vtkQtChartShapeLocatorNode *> &getNodes() const {return this->Nodes;}
  QList<vtkQtChartShapeLocatorNode *> &getNodes() {return this->Nodes;}

  void updateBounds();

private:
  QRectF *Bounds;                           ///< Stores the bounds.
  vtkQtChartShape *Element;                 ///< Stores the shape.
  vtkQtChartShapeLocatorNode *Parent;        ///< Stores the parent node.
  QList<vtkQtChartShapeLocatorNode *> Nodes; ///< Stores the child nodes.

private:
  vtkQtChartShapeLocatorNode(const vtkQtChartShapeLocatorNode &);
  vtkQtChartShapeLocatorNode &operator=(const vtkQtChartShapeLocatorNode &);
};


/// \class vtkQtChartShapeLocator
/// \brief
///   The vtkQtChartShapeLocator class is used to locate shapes using
///   a tree.
///
/// The tree is built from a table of shapes. The leaf nodes store the
/// shapes and use them for searches. The parent nodes in the tree use
/// the bounding rectangle for searches. The bounding rectangles can
/// be updated if the layout remains unchanged.
class VTKQTCHART_EXPORT vtkQtChartShapeLocator
{
public:
  vtkQtChartShapeLocator();
  ~vtkQtChartShapeLocator();

  /// Removes all the tree items.
  void clear();

  /// \brief
  ///   Builds a shape tree from the ordered table of shapes.
  ///
  /// The shape pointers are stored by the tree and should not be
  /// deleted until the tree has been cleared.
  ///
  /// \param table The ordered table of shapes.
  void build(const QList<QList<vtkQtChartShape *> > &table);

  /// \brief
  ///   Builds a shape tree from the list of shapes.
  ///
  /// The list of shapes should be sorted in the x-axis direction
  /// before calling this method. The list will be divided into a
  /// table and sorted in the y-axis direction before building the
  /// tree. The shape pointers are stored by the tree and should not
  /// be deleted until the tree has been cleared.
  ///
  /// \param list The list of shapes.
  void build(const QList<vtkQtChartShape *> &list);

  /// \brief
  ///   Updates the bounding rectangles in the shape tree.
  ///
  /// The nodes are traversed from last to first. The bounding
  /// rectangle of each node is updated using the shape if it is a
  /// leaf or the bounds of the children otherwise.
  void update();

  /// \brief
  ///   Gets the shapes at the specified point.
  /// \param point The point to search.
  /// \return
  ///   The list of shapes at the given point.
  QList<vtkQtChartShape *> getItemsAt(const QPointF &point) const;

  /// \brief
  ///   Gets the shapes in the specified rectangle.
  /// \param area The rectangle to search.
  /// \return
  ///   The list of shapes in the given rectangle.
  QList<vtkQtChartShape *> getItemsIn(const QRectF &area) const;

  /// \brief
  ///   Gets the last node in the tree.
  /// \return
  ///   A pointer to the last node in the tree.
  vtkQtChartShapeLocatorNode *getLast();

  /// \brief
  ///   Gets the previous node in the tree.
  /// \param node The node to search from.
  /// \return
  ///   A pointer to the previous node in the tree.
  vtkQtChartShapeLocatorNode *getPrevious(vtkQtChartShapeLocatorNode *node);

public:
  /// \brief
  ///   Sorts the list of shapes according to the y value.
  ///
  /// The list of shapes is sorted by the y-axis value using a quick
  /// sort algorithm. The list is sorted in place and non-recursively.
  ///
  /// \param list The list of shapes to be sorted.
  static void sort(QList<vtkQtChartShape *> &list);

private:
  /// \brief
  ///   Builds a shape tree from the ordered table of items.
  /// \param table the ordered table of items.
  void build(QLinkedList<QLinkedList<vtkQtChartShapeLocatorNode *> > &table);

private:
  vtkQtChartShapeLocatorNode *Root; ///< Stores the root node.

private:
  vtkQtChartShapeLocator(const vtkQtChartShapeLocator &);
  vtkQtChartShapeLocator &operator=(const vtkQtChartShapeLocator &);
};

#endif
