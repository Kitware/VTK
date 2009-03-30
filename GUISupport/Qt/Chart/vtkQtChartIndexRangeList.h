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


class VTKQTCHART_EXPORT vtkQtChartIndexRange
{
public:
  vtkQtChartIndexRange();
  vtkQtChartIndexRange(int first, int second, bool black=true);
  vtkQtChartIndexRange(const vtkQtChartIndexRange &other);
  ~vtkQtChartIndexRange();

  vtkQtChartIndexRange &operator=(const vtkQtChartIndexRange &other);

  /// \name Index Range Methods
  //@{
  int getFirst() const {return this->First;}
  void setFirst(int first);
  int getSecond() const {return this->Second;}
  void setSecond(int second);
  bool contains(int value) const;
  //@}

  /// \name Tree Methods
  //@{
  bool isBlack() const {return this->Black;}
  void setBlack(bool black) {this->Black = black;}
  vtkQtChartIndexRange *getParent() const {return this->Parent;}
  void setParent(vtkQtChartIndexRange *parent) {this->Parent = parent;}
  vtkQtChartIndexRange *getLeft() const {return this->Left;}
  void setLeft(vtkQtChartIndexRange *left) {this->Left = left;}
  vtkQtChartIndexRange *getRight() const {return this->Right;}
  void setRight(vtkQtChartIndexRange *right) {this->Right = right;}
  //@}

private:
  vtkQtChartIndexRange *Parent;
  vtkQtChartIndexRange *Left;
  vtkQtChartIndexRange *Right;
  bool Black;
  int First;
  int Second;
};

class VTKQTCHART_EXPORT vtkQtChartIndexRangeList
{
public:
  vtkQtChartIndexRangeList();
  vtkQtChartIndexRangeList(int first, int second);
  vtkQtChartIndexRangeList(const vtkQtChartIndexRangeList &other);
  ~vtkQtChartIndexRangeList();

  vtkQtChartIndexRangeList &operator=(const vtkQtChartIndexRangeList &other);

  bool isEmpty() const {return this->Root == 0;}

  vtkQtChartIndexRange *getFirst() const;

  vtkQtChartIndexRange *getLast() const;

  vtkQtChartIndexRange *getNext(vtkQtChartIndexRange *range) const;

  vtkQtChartIndexRange *getPrevious(vtkQtChartIndexRange *range) const;

  vtkQtChartIndexRange *findClosest(int value) const;

  bool contains(int value) const;

  bool clear();

  bool setRange(int first, int second);

  bool setRanges(const vtkQtChartIndexRangeList &ranges);

  bool addRange(int first, int second);

  bool addRanges(const vtkQtChartIndexRangeList &ranges);

  bool subtractRange(int first, int second);

  bool subtractRanges(const vtkQtChartIndexRangeList &ranges);

  bool xorRange(int first, int second);

  bool xorRanges(const vtkQtChartIndexRangeList &ranges);

  void limitRange(int minimum, int maximum);

  bool offsetRanges(int start, int offset);

private:
  vtkQtChartIndexRange *findNode(int value, bool left) const;
  void rotateLeft(vtkQtChartIndexRange *node);
  void rotateRight(vtkQtChartIndexRange *node);
  void insertNode(vtkQtChartIndexRange *current, vtkQtChartIndexRange *node,
      bool left);
  void removeNode(vtkQtChartIndexRange *node);
  int removeBetween(vtkQtChartIndexRange *left, vtkQtChartIndexRange *right);
  vtkQtChartIndexRange *getNextNode(vtkQtChartIndexRange *node) const;
  vtkQtChartIndexRange *getPreviousNode(vtkQtChartIndexRange *node) const;
  vtkQtChartIndexRange *getFirstNode(vtkQtChartIndexRange *root) const;
  vtkQtChartIndexRange *getLastNode(vtkQtChartIndexRange *root) const;

private:
  vtkQtChartIndexRange *Root;
};

#endif
