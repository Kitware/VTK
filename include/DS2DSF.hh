/*=========================================================================

  Program:   Visualization Toolkit
  Module:    DS2DSF.hh
  Language:  C++
  Date:      2/17/94
  Version:   1.8


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
// .NAME vtkDataSetToDataSetFilter - abstract filter class
// .SECTION Description
// vtkDataSetToDataSetFilter is an abstract filter class. Subclasses of 
// vtkDataSetToDataSetFilter take a dataset as input and create a dataset 
// as output. The form of the input geometry is not changed in these 
// filters, only the point attributes (e,g,, scalars, vectors, etc.).

#ifndef __vtkDataSetToDataSetFilter_h
#define __vtkDataSetToDataSetFilter_h

#include "DataSetF.hh"
#include "DataSet.hh"

class vtkDataSetToDataSetFilter : public vtkDataSet, public vtkDataSetFilter
{
public:
  vtkDataSetToDataSetFilter();
  ~vtkDataSetToDataSetFilter();
  char *GetClassName() {return "vtkDataSetToDataSetFilter";};
  char *GetDataType() {return this->DataSet->GetDataType();};
  void PrintSelf(ostream& os, vtkIndent indent);

  // dataset interface
  vtkDataSet *MakeObject() {return this->DataSet->MakeObject();};
  int GetNumberOfCells() {return this->DataSet->GetNumberOfCells();}
  int GetNumberOfPoints() {return this->DataSet->GetNumberOfPoints();}
  float *GetPoint(int i) {return this->DataSet->GetPoint(i);}
  void GetPoint(int i, float p[3]) {this->DataSet->GetPoint(i,p);}
  vtkCell *GetCell(int cellId) {return this->DataSet->GetCell(cellId);}
  int GetCellType(int cellId) {return this->DataSet->GetCellType(cellId);}
  void Initialize();
  void GetCellPoints(int cellId, vtkIdList& ptIds) {this->DataSet->GetCellPoints(cellId, ptIds);};
  void GetPointCells(int ptId, vtkIdList& cellIds) {this->DataSet->GetPointCells(ptId, cellIds);};
  int FindCell(float x[3], vtkCell *cell, float tol2, int& subId, 
               float pc[3], float weights[MAX_CELL_SIZE])
    {return this->DataSet->FindCell(x,cell,tol2,subId,pc,weights);};
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
  vtkDataSet *DataSet;

  //Filter interface
  int GetDataReleased();
  void SetDataReleased(int flag);
};



#endif


