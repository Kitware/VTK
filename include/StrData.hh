/*=========================================================================

  Program:   Visualization Toolkit
  Module:    StrData.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1995 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.


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
