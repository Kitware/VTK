/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFloatTCoords.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1996 Ken Martin, Will Schroeder, Bill Lorensen.

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
// .NAME vtkFloatTCoords - floating point representation of texture coordinates
// .SECTION Description
// vtkFloatTCoords is a concrete implementation of vtkTCoords. Texture
// coordinates are represented using float values.

#ifndef __vtkFloatTCoords_h
#define __vtkFloatTCoords_h

#include "vtkTCoords.h"
#include "vtkFloatArray.h"

class VTK_EXPORT vtkFloatTCoords : public vtkTCoords
{
public:
  vtkFloatTCoords();
  vtkFloatTCoords(const vtkFloatTCoords& ftc);
  vtkFloatTCoords(int sz, int d=2, int ext=1000);
  ~vtkFloatTCoords();

  int Allocate(const int sz, const int dim=2, const int ext=1000) {return this->TC->Allocate(dim*sz,dim*ext);};
  void Initialize() {this->TC->Initialize();};
  char *GetClassName() {return "vtkFloatTCoords";};

  // vtkTCoords interface
  vtkTCoords *MakeObject(int sze, int d=2, int ext=1000);
  char *GetDataType() {return "float";};
  int GetNumberOfTCoords() {return (this->TC->GetMaxId()+1)/this->Dimension;};
  void Squeeze() {this->TC->Squeeze();};
  float *GetTCoord(int i) {return this->TC->GetPtr(this->Dimension*i);};
  void GetTCoord(int i,float tc[3]) {this->vtkTCoords::GetTCoord(i,tc);};
  void SetNumberOfTCoords(int number);
  void SetTCoord(int i, float *tc);
  void InsertTCoord(int i, float *tc);
  int InsertNextTCoord(float *tc);

  // miscellaneous
  float *GetPtr(const int id);
  float *WritePtr(const int id, const int number);
  vtkFloatTCoords &operator=(const vtkFloatTCoords& ftc);
  void operator+=(const vtkFloatTCoords& ftc) {*(this->TC) += *(ftc.TC);};
  void Reset() {this->TC->Reset();};

protected:
  vtkFloatArray *TC;
};


// Description:
// Get pointer to array of data starting at data position "id".
inline float *vtkFloatTCoords::GetPtr(const int id)
{
  return this->TC->GetPtr(id);
}

// Description:
// Get pointer to data array. Useful for direct writes of data. MaxId is 
// bumped by number (and memory allocated if necessary). Id is the 
// location you wish to write into; number is the number of texture coordinates to 
// write. Make sure the dimension of the texture coordinate is set prior to issuing 
// this call.
inline float *vtkFloatTCoords::WritePtr(const int id, const int number)
{
  return this->TC->WritePtr(id,this->Dimension*number);
}

inline void vtkFloatTCoords::SetNumberOfTCoords(int number)
{
  this->TC->SetNumberOfValues(number*this->Dimension);
}

inline void vtkFloatTCoords::SetTCoord(int i, float *tc) 
{
  i*=this->Dimension; 
  for(int j=0;j<this->Dimension;j++) this->TC->SetValue(i+j,tc[j]);
}

inline void vtkFloatTCoords::InsertTCoord(int i, float *tc) 
{
  i*=this->Dimension; 
  for(int j=0; j<this->Dimension; j++) this->TC->InsertValue(i+j, tc[j]);
}

inline int vtkFloatTCoords::InsertNextTCoord(float *tc) 
{
  int id = this->TC->InsertNextValue(tc[0]);
  for(int j=1; j<this->Dimension; j++) this->TC->InsertNextValue(tc[j]);
  return id/this->Dimension;
}

#endif
