/*=========================================================================

  Program:   Visualization Toolkit
  Module:    StrData.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vtkStructuredData - abstract class for topologically regular data
// .SECTION Description
// vtkStructuredData is an abstract class that specifies an interface for
// topologically regular data. Regular data is data that can be accessed
// in rectangular fashion using a i-j-k index. A finite difference grid,
// a volume, or a pixmap are all considered regular.

#ifndef __vtkStructuredData_h
#define __vtkStructuredData_h

#include "LWObject.hh"
#include "BArray.hh"
#include "IdList.hh"

#define SINGLE_POINT 0
#define X_LINE 1
#define Y_LINE 2
#define Z_LINE 3
#define XY_PLANE 4
#define YZ_PLANE 5
#define XZ_PLANE 6
#define XYZ_GRID 7

class vtkStructuredData : public vtkLWObject 
{
public:
  vtkStructuredData();
  vtkStructuredData(const vtkStructuredData& sds);
  virtual ~vtkStructuredData();
  void _PrintSelf(ostream& os, vtkIndent indent);

  // setting object dimensions
  void SetDimensions(int i, int j, int k);
  void SetDimensions(int dim[3]);
  int *GetDimensions();
  void GetDimensions(int dim[3]);

  int GetDataDimension();

  void BlankingOn();
  void BlankingOff();
  int GetBlanking() {return this->Blanking;};
  void BlankPoint(int ptId);
  void UnBlankPoint(int ptId);
  int IsPointVisible(int ptId);

protected:
  // methods to support datasets (done because of MI problems)
  int _GetNumberOfCells();
  int _GetNumberOfPoints(); 
  void _Initialize();
  void _GetCellPoints(int cellId, vtkIdList& ptIds);
  void _GetPointCells(int ptId, vtkIdList& cellIds);

  int Dimensions[3];
  int DataDescription;
  int Blanking;
  vtkBitArray *PointVisibility;
};

// Description:
// Return non-zero value if specified point is visible.
inline int vtkStructuredData::IsPointVisible(int ptId) 
{
  if (!this->Blanking) return 1; 
  else return this->PointVisibility->GetValue(ptId);
}

#endif
