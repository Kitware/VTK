/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTCoords.h
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
// .NAME vtkTCoords - abstract interface to texture coordinates
// .SECTION Description
// vtkTCoords provides an abstract interface to 2D or 3D texture coordinates. 
// Texture coordinates are 2D (s,t) or 3D (r,s,t) parametric values that
// map geometry into regular 2D or 3D arrays of color and/or transparency
// values. During rendering the array are mapped onto the geometry for
// fast image detailing. The subclasses of vtkTCoords are concrete data 
// types (float, int, etc.) that implement the interface of vtkTCoords. 

#ifndef __vtkTCoords_h
#define __vtkTCoords_h

#include "vtkRefCount.h"

class vtkIdList;
class vtkFloatTCoords;

class VTK_EXPORT vtkTCoords : public vtkRefCount
{
public:
  vtkTCoords(int dim=2);
  char *GetClassName() {return "vtkTCoords";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create a copy of this object.
  virtual vtkTCoords *MakeObject(int sze, int d=2, int ext=1000) = 0;

  // Description:
  // Return data type. One of "bit", "unsigned char", "short", "int", "float", or
  // "double".
  virtual char *GetDataType() = 0;

  // Description:
  // Return number of texture coordinates in array.
  virtual int GetNumberOfTCoords() = 0;

  // Description:
  // Return a float texture coordinate tc[2/3] for a particular point id.
  virtual float *GetTCoord(int id) = 0;

  // Description:
  // Copy float texture coordinates into user provided array tc[2/3] 
  // for specified point id.
  virtual void GetTCoord(int id, float tc[3]);

  // Description:
  // Insert texture coordinate into object. No range checking performed (fast!).
  virtual void SetTCoord(int id, float *tc) = 0;

  // Description:
  // Insert texture coordinate into object. Range checking performed and 
  // memory allocated as necessary.
  virtual void InsertTCoord(int id, float *tc) = 0;
  void InsertTCoord(int id, float tc1, float tc2, float tc3);

  // Description:
  // Insert texture coordinate into next available slot. Returns point
  // id of slot.
  virtual int InsertNextTCoord(float *tc) = 0;
  int InsertNextTCoord(float tc1, float tc2, float tc3);

  // Description:
  // Reclaim any extra memory.
  virtual void Squeeze() = 0;

  void GetTCoords(vtkIdList& ptId, vtkFloatTCoords& fp);

  vtkSetClampMacro(Dimension,int,1,3);
  vtkGetMacro(Dimension,int);

protected:
  int Dimension;

};

// These include files are placed here so that if TCoords.h is included 
// all other classes necessary for compilation are also included. 
#include "vtkIdList.h"
#include "vtkFloatTCoords.h"

#endif
