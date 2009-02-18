/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtChartBarLocator.h

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

/// \file vtkQtChartBarLocator.h
/// \date October 1, 2008

#ifndef _vtkQtChartBarLocator_h
#define _vtkQtChartBarLocator_h

#include "vtkQtChartExport.h"
#include <QLinkedList> // Needed for parameter.
#include <QList> // Needed for parameter.

class vtkQtChartBar;
class QPointF;
class QRectF;


/// \class vtkQtChartBarLocatorNode
/// \brief
///   The vtkQtChartBarLocatorNode class is used to build a tree of
///   chart bar shapes.
class VTKQTCHART_EXPORT vtkQtChartBarLocatorNode
{
public:
  /// \brief
  ///   Constructs a chart bar locator node.
  /// \param element The chart bar element to wrap.
  vtkQtChartBarLocatorNode(vtkQtChartBar *element=0);
  ~vtkQtChartBarLocatorNode();

  /// \brief
  ///   Gets whether or not the bounding area contains the given point.
  /// \param point The position to evaluate.
  /// \return
  ///   True if the bounding area contains the given point.
  bool contains(const QPointF &point) const;

  /// \brief
  ///   Gets whether or not the bounding area intersects the given area.
  /// \param area The rectangle to evaluate.
  /// \return
  ///   True if the bounding area intersects the given area.
  bool intersects(const QRectF &area) const;

  /// \brief
  ///   Gets the bounding rectangle.
  /// \return
  ///   The bounding rectangle.
  const QRectF &getBounds() const {return *this->Bounds;}

  /// \brief
  ///   Sets the bounding rectangle.
  /// \param bounds The new bounding rectangle.
  void setBounds(const QRectF &bounds);

  /// \brief
  ///   Gets the chart shape element.
  /// \return
  ///   A pointer to the chart shape element.
  vtkQtChartBar *getElement() const {return this->Element;}

  /// \brief
  ///   Sets the chart shape element.
  /// \param element The new chart shape element.
  void setElement(vtkQtChartBar *element);

  /// \brief
  ///   Gets the parent node.
  /// \return
  ///   A pointer to the parent node.
  vtkQtChartBarLocatorNode *getParent() const {return this->Parent;}

  /// \brief
  ///   Sets the parent node.
  /// \param parent The new parent node.
  void setParent(vtkQtChartBarLocatorNode *parent) {this->Parent = parent;}

  /// \brief
  ///   Gets whether or not the node has children.
  /// \return
  ///   True if the node has children.
  bool hasChildren() const {return this->First || this->Second;}

  /// \brief
  ///   Gets the first child node.
  /// \return
  ///   A pointer to the first child node.
  vtkQtChartBarLocatorNode *getFirst() const {return this->First;}

  /// \brief
  ///   Sets the first child node.
  /// \param first The first child node.
  void setFirst(vtkQtChartBarLocatorNode *first) {this->First = first;}

  /// \brief
  ///   Gets the second child node.
  /// \return
  ///   A pointer to the second child node.
  vtkQtChartBarLocatorNode *getSecond() const {return this->Second;}

  /// \brief
  ///   Sets the second child node.
  /// \param second The second child node.
  void setSecond(vtkQtChartBarLocatorNode *second) {this->Second = second;}

  /// \brief
  ///   Updates the bounding rectangle.
  ///
  /// The bounding rectangle is the bounds of the element or the
  /// bounds of the child nodes.
  void updateBounds();

private:
  QRectF *Bounds;                   ///< Stores the bounding rectangle.
  vtkQtChartBar *Element;           ///< Stores the shape element.
  vtkQtChartBarLocatorNode *Parent; ///< Stores the parent node.
  vtkQtChartBarLocatorNode *First;  ///< Stores the first child node.
  vtkQtChartBarLocatorNode *Second; ///< Stores the second child node.

private:
  vtkQtChartBarLocatorNode(const vtkQtChartBarLocatorNode &);
  vtkQtChartBarLocatorNode &operator=(const vtkQtChartBarLocatorNode &);
};


/// \class vtkQtChartBarLocator
/// \brief
///   The vtkQtChartBarLocator class is used to locate bars in a tree
///   structure.
///
/// The tree is built from a list of bars. The leaf nodes store the
/// bars. The bounding rectangles can be updated if the layout
/// remains unchanged.
class VTKQTCHART_EXPORT vtkQtChartBarLocator
{
public:
  vtkQtChartBarLocator();
  ~vtkQtChartBarLocator();

  /// Removes all the tree items.
  void clear();

  /// \brief
  ///   Builds a bar tree from the ordered list of rectangles.
  ///
  /// The bar pointers are stored by the tree and should not be
  /// deleted until the tree has been cleared.
  ///
  /// \param list The ordered list of rectangles.
  void build(const QList<vtkQtChartBar *> &list);

  /// \brief
  ///   Updates the bounding rectangles in the bar tree.
  ///
  /// The nodes are traversed from last to first. The bounding
  /// rectangle of each node is updated using the bar if it is a leaf
  /// or the bounds of the children otherwise.
  void update();

  /// \brief
  ///   Gets the bar at the specified point.
  /// \param point The point to search.
  /// \return
  ///   A pointer to the bar at the given point.
  vtkQtChartBar *getItemAt(const QPointF &point) const;

  /// \brief
  ///   Gets the bars in the specified rectangle.
  /// \param area The rectangle to search.
  /// \return
  ///   The list of bars in the given rectangle.
  QList<vtkQtChartBar *> getItemsIn(const QRectF &area) const;

  /// \brief
  ///   Gets the last node in the tree.
  /// \return
  ///   A pointer to the last node in the tree.
  vtkQtChartBarLocatorNode *getLast();

  /// \brief
  ///   Gets the previous node in the tree.
  /// \param node The node to search from.
  /// \return
  ///   A pointer to the previous node in the tree.
  vtkQtChartBarLocatorNode *getPrevious(vtkQtChartBarLocatorNode *node);

private:
  /// \brief
  ///   Builds a bar tree from the ordered list of items.
  /// \param list The ordered list of items.
  void buildTree(QLinkedList<vtkQtChartBarLocatorNode *> &list) const;

private:
  vtkQtChartBarLocatorNode *Root;          ///< Stores the tree root.
  QList<vtkQtChartBarLocatorNode *> Items; ///< Stores the item list.

private:
  vtkQtChartBarLocator(const vtkQtChartBarLocator &);
  vtkQtChartBarLocator &operator=(const vtkQtChartBarLocator &);
};

#endif
