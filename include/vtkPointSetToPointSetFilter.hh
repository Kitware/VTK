/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPointSetToPointSetFilter.hh
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


=========================================================================*/
// .NAME vtkPointSetToPointSetFilter - abstract filter class 
// .SECTION Description
// vtkPointSetToPointSetFilter is an abstract filter class whose subclasses
// take as input a point set and generates a point set on output.
// At a minimum the concrete subclasses of vtkPointSetToPointSetFilter
// modify their point coordinates. They never modify their topological 
// form, however.

#ifndef __vtkPointSetToPointSetFilter_h
#define __vtkPointSetToPointSetFilter_h

#include "vtkPointSetFilter.hh"
#include "vtkPointSet.hh"

class vtkPointSetToPointSetFilter : public vtkPointSet, public vtkPointSetFilter
{
public:
  vtkPointSetToPointSetFilter();
  ~vtkPointSetToPointSetFilter();
  char *GetDataType() {return this->PointSet->GetDataType();};
  char *GetClassName() {return "vtkPointSetToPointSetFilter";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // dataset interface
  vtkDataSet *MakeObject();
  int GetNumberOfCells() {return this->PointSet->GetNumberOfCells();};
  vtkCell *GetCell(int cellId) {return this->PointSet->GetCell(cellId);};
  int GetCellType(int cellId) {return this->PointSet->GetCellType(cellId);};
  void GetCellPoints(int cellId, vtkIdList& ptIds)
    {this->PointSet->GetCellPoints(cellId, ptIds);};
  void GetPointCells(int ptId, vtkIdList& cellIds)
    {this->PointSet->GetPointCells(ptId, cellIds);};
  void Initialize();

  void ComputeBounds();

  // Object interface
  void Modified();
  unsigned long int GetMTime();
  unsigned long int _GetMTime() {return this->GetMTime();};
  void DebugOn();
  void DebugOff();

  //DataSet interface
  void Update();

protected:
  vtkDataSet *PointSet;

  //Filter interface
  int GetDataReleased();
  void SetDataReleased(int flag);
};

#endif


