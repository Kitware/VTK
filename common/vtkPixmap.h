/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPixmap.h
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
// .NAME vtkPixmap - scalar data in RGB (color) form
// .SECTION Description
// vtkPixmap is a concrete implementation of vtkScalars. Scalars are
// represented using three values for color (red, green, blue). Each of
// r,g,b ranges from (0,255) (i.e., an unsigned char value).

#ifndef __vtkPixmap_h
#define __vtkPixmap_h

#include "vtkColorScalars.h"
#include "vtkUnsignedCharArray.h"

class VTK_EXPORT vtkPixmap : public vtkColorScalars 
{
public:
  vtkPixmap() {};
  vtkPixmap(const vtkPixmap& fs) {this->S = fs.S;};
  vtkPixmap(const int sz, const int ext=1000):S(3*sz,3*ext){};
  int Allocate(const int sz, const int ext=1000) {return this->S.Allocate(3*sz,3*ext);};
  void Initialize() {this->S.Initialize();};
  char *GetClassName() {return "vtkPixmap";};

  // vtkScalar interface
  vtkScalars *MakeObject(int sze, int ext=1000);
  int GetNumberOfScalars() {return (this->S.GetMaxId()+1)/3;};
  void Squeeze() {this->S.Squeeze();};
  int GetNumberOfValuesPerScalar() {return 3;};

  // miscellaneous
  vtkPixmap &operator=(const vtkPixmap& fs);
  void operator+=(const vtkPixmap& fs) {this->S += fs.S;};
  void Reset() {this->S.Reset();};
  unsigned char *GetPtr(const int id);
  unsigned char *WritePtr(const int id, const int number);

  // vtkColorScalar interface.
  unsigned char *GetColor(int id);
  void GetColor(int id, unsigned char rgba[4]);
  void SetNumberOfColors(int number);
  void SetColor(int id, unsigned char rgba[4]);
  void InsertColor(int id, unsigned char rgba[4]);
  int InsertNextColor(unsigned char rgba[4]);

protected:
  vtkUnsignedCharArray S;
};

// Description:
// Set a rgba color value at a particular array location. Does not do 
// range checking. Make sure you use SetNumberOfColors() to allocate 
// memory prior to using SetColor().
inline void vtkPixmap::SetColor(int i, unsigned char rgba[4]) 
{
  i *= 3; 
  memcpy (this->S.GetPtr(i), rgba, 3);
}

// Description:
// Insert a rgba color value at a particular array location. Does range 
// checking and will allocate additional memory if necessary.
inline void vtkPixmap::InsertColor(int i, unsigned char rgba[4]) 
{
  i *= 3;
  this->S.InsertValue(i+2, rgba[2]);
  this->S.SetValue(i, rgba[0]);
  this->S.SetValue(i+1, rgba[1]);
}

// Description:
// Insert a rgba value at the next available slot in the array. Will allocate
// memory if necessary.
inline int vtkPixmap::InsertNextColor(unsigned char *rgba) 
{
  int id = this->S.GetMaxId() + 3;
  this->S.InsertValue(id,rgba[2]);
  this->S.SetValue(id-2, rgba[0]);
  this->S.SetValue(id-1, rgba[1]);
  return id/3;
}

// Description:
// Get pointer to array of data starting at data position "id".
inline unsigned char *vtkPixmap::GetPtr(const int id)
{
  return this->S.GetPtr(3*id);
}

// Description:
// Get pointer to data array. Useful for direct writes of data. MaxId is 
// bumped by number (and memory allocated if necessary). Id is the 
// location you wish to write into; number is the number of scalars to 
// write. 
inline unsigned char *vtkPixmap::WritePtr(const int id, const int number)
{
  return this->S.WritePtr(3*id,3*number);
}

#endif
