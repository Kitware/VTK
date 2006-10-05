/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTreeMapLayoutStrategy.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkTreeMapLayoutStrategy - abstract superclass for all tree map layout strategies
//
// .SECTION Description
//
// .SECTION Thanks
// Thanks to Brian Wylie and Ken Moreland from Sandia National Laboratories
// for help developing this class.

#ifndef __vtkTreeMapLayoutStrategy_h
#define __vtkTreeMapLayoutStrategy_h


#include "vtkObject.h"

class vtkTree;
class vtkDataArray;

class VTK_INFOVIS_EXPORT vtkTreeMapLayoutStrategy : public vtkObject 
{
public:
  vtkTypeRevisionMacro(vtkTreeMapLayoutStrategy,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  virtual void Layout(vtkTree *inputTree, vtkDataArray *rectArray) = 0;
  vtkSetMacro(BorderPercentage, double);
  vtkGetMacro(BorderPercentage, double);

protected:
  vtkTreeMapLayoutStrategy();
  ~vtkTreeMapLayoutStrategy();
  double BorderPercentage;
  void AddBorder( float *boxInfo);
private:
  vtkTreeMapLayoutStrategy(const vtkTreeMapLayoutStrategy&);  // Not implemented.
  void operator=(const vtkTreeMapLayoutStrategy&);  // Not implemented.
};

#endif

