/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOBBDicer.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkOBBDicer - divide dataset into spatially aggregated pieces
// .SECTION Description
// vtkOBBDicer separates the cells of a dataset into spatially
// aggregated pieces using a Oriented Bounding Box (OBB). These pieces
// can then be operated on by other filters (e.g., vtkThreshold). One
// application is to break very large polygonal models into pieces and
// performing viewing and occlusion culling on the pieces.
//
// Refer to the superclass documentation (vtkDicer) for more information.

// .SECTION See Also
// vtkDicer vtkConnectedDicer

#ifndef __vtkOBBDicer_h
#define __vtkOBBDicer_h

#include "vtkDicer.h"

class vtkOBBNode;
class vtkShortArray;
class vtkIdList;
class vtkPoints;

class VTK_GRAPHICS_EXPORT vtkOBBDicer : public vtkDicer 
{
public:
  vtkTypeMacro(vtkOBBDicer,vtkDicer);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Instantiate an object.
  static vtkOBBDicer *New();

protected:
  vtkOBBDicer() {};
  ~vtkOBBDicer() {};

  // Usual data generation method
  virtual int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);

  //implementation ivars and methods
  void BuildTree(vtkIdList *ptIds, vtkOBBNode *OBBptr, vtkDataSet *input);
  void MarkPoints(vtkOBBNode *OBBptr, vtkShortArray *groupIds);
  void DeleteTree(vtkOBBNode *OBBptr);
  vtkPoints *PointsList;
  
private:
  vtkOBBDicer(const vtkOBBDicer&);  // Not implemented.
  void operator=(const vtkOBBDicer&);  // Not implemented.
};

#endif


