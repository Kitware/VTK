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
/**
 * @class   vtkOBBDicer
 * @brief   divide dataset into spatially aggregated pieces
 *
 * vtkOBBDicer separates the cells of a dataset into spatially
 * aggregated pieces using a Oriented Bounding Box (OBB). These pieces
 * can then be operated on by other filters (e.g., vtkThreshold). One
 * application is to break very large polygonal models into pieces and
 * performing viewing and occlusion culling on the pieces.
 *
 * Refer to the superclass documentation (vtkDicer) for more information.
 *
 * @sa
 * vtkDicer vtkConnectedDicer
*/

#ifndef vtkOBBDicer_h
#define vtkOBBDicer_h

#include "vtkFiltersGeneralModule.h" // For export macro
#include "vtkDicer.h"

class vtkOBBNode;
class vtkShortArray;
class vtkIdList;
class vtkPoints;

class VTKFILTERSGENERAL_EXPORT vtkOBBDicer : public vtkDicer
{
public:
  vtkTypeMacro(vtkOBBDicer,vtkDicer);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Instantiate an object.
   */
  static vtkOBBDicer *New();

protected:
  vtkOBBDicer() {}
  ~vtkOBBDicer() override {}

  // Usual data generation method
  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *) override;

  //implementation ivars and methods
  void BuildTree(vtkIdList *ptIds, vtkOBBNode *OBBptr, vtkDataSet *input);
  void MarkPoints(vtkOBBNode *OBBptr, vtkShortArray *groupIds);
  void DeleteTree(vtkOBBNode *OBBptr);
  vtkPoints *PointsList;

private:
  vtkOBBDicer(const vtkOBBDicer&) = delete;
  void operator=(const vtkOBBDicer&) = delete;
};

#endif


