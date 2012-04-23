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

#include "vtkRenderingLabelModule.h" // For export macro
#include "vtkLabelHierarchyAlgorithm.h"

class vtkTextProperty;

class VTKRENDERINGLABEL_EXPORT vtkPointSetToLabelHierarchy : public vtkLabelHierarchyAlgorithm
{
public:
  static vtkPointSetToLabelHierarchy* New();
  vtkTypeMacro(vtkPointSetToLabelHierarchy,vtkLabelHierarchyAlgorithm);
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
  // Whether to use unicode strings.
  vtkSetMacro(UseUnicodeStrings,bool);
  vtkGetMacro(UseUnicodeStrings,bool);
  vtkBooleanMacro(UseUnicodeStrings,bool);

  // Description:
  // Set/get the label array name.
  virtual void SetLabelArrayName(const char* name);
  virtual const char* GetLabelArrayName();

  // Description:
  // Set/get the priority array name.
  virtual void SetSizeArrayName(const char* name);
  virtual const char* GetSizeArrayName();

  // Description:
  // Set/get the priority array name.
  virtual void SetPriorityArrayName(const char* name);
  virtual const char* GetPriorityArrayName();

  // Description:
  // Set/get the icon index array name.
  virtual void SetIconIndexArrayName(const char* name);
  virtual const char* GetIconIndexArrayName();

  // Description:
  // Set/get the text orientation array name.
  virtual void SetOrientationArrayName(const char* name);
  virtual const char* GetOrientationArrayName();

  // Description:
  // Set/get the maximum text width (in world coordinates) array name.
  virtual void SetBoundedSizeArrayName(const char* name);
  virtual const char* GetBoundedSizeArrayName();

  // Description:
  // Set/get the text property assigned to the hierarchy.
  virtual void SetTextProperty(vtkTextProperty* tprop);
  vtkGetObjectMacro(TextProperty, vtkTextProperty);

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
  vtkTextProperty* TextProperty;

private:
  vtkPointSetToLabelHierarchy( const vtkPointSetToLabelHierarchy& ); // Not implemented.
  void operator = ( const vtkPointSetToLabelHierarchy& ); // Not implemented.
};

#endif // __vtkPointSetToLabelHierarchy_h
