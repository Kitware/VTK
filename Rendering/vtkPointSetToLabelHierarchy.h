/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPointSetToLabelHierarchy.h

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
// .NAME vtkPointSetToLabelHierarchy - build a label hierarchy for a graph or point set.
//
// .SECTION Description
//
// Every point in the input vtkPoints object is taken to be an
// anchor point for a label. Statistics on the input points
// are used to subdivide an octree referencing the points
// until the points each octree node contains have a variance
// close to the node size and a limited population (< 100).

#ifndef __vtkPointSetToLabelHierarchy_h
#define __vtkPointSetToLabelHierarchy_h

#include "vtkLabelHierarchyAlgorithm.h"

class VTK_RENDERING_EXPORT vtkPointSetToLabelHierarchy : public vtkLabelHierarchyAlgorithm
{
public:
  static vtkPointSetToLabelHierarchy* New();
  vtkTypeRevisionMacro(vtkPointSetToLabelHierarchy,vtkLabelHierarchyAlgorithm);
  virtual void PrintSelf( ostream& os, vtkIndent indent );

  // Description:
  // Set/get the "ideal" number of labels to associate with each node in the output hierarchy.
  vtkSetMacro(TargetLabelCount,int);
  vtkGetMacro(TargetLabelCount,int);

  // Description:
  // Set/get the maximum tree depth in the output hierarchy.
  vtkSetMacro(MaximumDepth,int);
  vtkGetMacro(MaximumDepth,int);

  // Description:
  // Set whether, or not, to use unicode strings.
  vtkSetMacro(UseUnicodeStrings,bool);
  vtkGetMacro(UseUnicodeStrings,bool);
  vtkBooleanMacro(UseUnicodeStrings,bool);

protected:
  vtkPointSetToLabelHierarchy();
  virtual ~vtkPointSetToLabelHierarchy();

  virtual int FillInputPortInformation( int port, vtkInformation* info );

  virtual int RequestData(
    vtkInformation* request,
    vtkInformationVector** inputVector,
    vtkInformationVector* outputVector );

  int TargetLabelCount;
  int MaximumDepth;
  bool UseUnicodeStrings;

private:
  vtkPointSetToLabelHierarchy( const vtkPointSetToLabelHierarchy& ); // Not implemented.
  void operator = ( const vtkPointSetToLabelHierarchy& ); // Not implemented.
};

#endif // __vtkPointSetToLabelHierarchy_h
