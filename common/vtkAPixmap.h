/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAPixmap.h
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
// .NAME vtkAPixmap - scalar data in rgba (color + opacity) form
// .SECTION Description
// vtkAPixmap is a concrete implementation of vtkColorScalars. Scalars are
// represented using three values for color (red, green, blue) plus alpha
// opacity value. Each of r,g,b,a components ranges from (0,255) (i.e.,
// an unsigned char value).
// .SECTION See Also
// vtkGraymap vtkAGraymap vtkPixmap vtkBitmap

#ifndef __vtkAPixmap_h
#define __vtkAPixmap_h

#include "vtkColorScalars.hh"
#include "vtkUnsignedCharArray.hh"

class vtkAPixmap : public vtkColorScalars 
{
public:
  vtkAPixmap() {};
  vtkAPixmap(const vtkAPixmap& fs) {this->S = fs.S;};
  vtkAPixmap(const int sz, const int ext=1000):S(4*sz,4*ext){};
  int Allocate(const int sz, const int ext=1000) {return this->S.Allocate(4*sz,4*ext);};
  void Initialize() {this->S.Initialize();};
  char *GetClassName() {return "vtkAPixmap";};

  // vtkScalar interface
  vtkScalars *MakeObject(int sze, int ext=1000);
  int GetNumberOfScalars() {return (this->S.GetMaxId()+1)/4;};
  void Squeeze() {this->S.Squeeze();};
  int GetNumberOfValuesPerScalar() {return 4;};

  // miscellaneous
  vtkAPixmap &operator=(const vtkAPixmap& fs);
  void operator+=(const vtkAPixmap& fs) {this->S += fs.S;};
  void Reset() {this->S.Reset();};
  unsigned char *GetPtr(const int id);
  unsigned char *WritePtr(const int id, const int number);
  void WrotePtr();

  // vtkColorScalar interface.
  unsigned char *GetColor(int id);
  void GetColor(int id, unsigned char rgba[4]);
  void SetColor(int id, unsigned char rgba[4]);
  void InsertColor(int id, unsigned char rgba[4]);
  int InsertNextColor(unsigned char rgba[4]);

protected:
  vtkUnsignedCharArray S;
};

// Description:
// Return a rgba color at array location i.
inline unsigned char *vtkAPixmap::GetColor(int i) 
{
  return this->S.GetPtr(4*i);
}

// Description:
// Get pointer to array of data starting at data position "id".
inline unsigned char *vtkAPixmap::GetPtr(const int id)
{
  return this->S.GetPtr(4*id);
}

// Description:
// Get pointer to data array. Useful for direct writes of data. MaxId is 
// bumped by number (and memory allocated if necessary). Id is the 
// location you wish to write into; number is the number of scalars to 
// write. Use the method WrotePtr() to mark completion of write.
inline unsigned char *vtkAPixmap::WritePtr(const int id, const int number)
{
  return this->S.WritePtr(4*id,4*number);
}

// Description:
// Terminate direct write of data. Although dummy routine now, reserved for
// future use.
inline void vtkAPixmap::WrotePtr() {}

#endif
