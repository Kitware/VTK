/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBitmap.h
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
// .NAME vtkBitmap - scalar data in bitmap form
// .SECTION Description
// vtkBitmap is a concrete implementation of vtkColorScalars. Scalars are
// represented using a packed character array of (0,1) values.
//
// If you use the method SetColor() (inherited method) the conversion to bit 
// value is as follows. Any non-black color is set "on" and black is set "off".
// .SECTION See Also
// vtkGraymap vtkAGraymap vtkPixmap vtkAPixmap 

#ifndef __vtkBitmap_h
#define __vtkBitmap_h

#include "vtkColorScalars.h"
#include "vtkBitArray.h"

class VTK_EXPORT vtkBitmap : public vtkColorScalars 
{
public:
  vtkBitmap();
  vtkBitmap(const vtkBitmap& fs);
  vtkBitmap(const int sz, const int ext=1000);
  ~vtkBitmap();

  int Allocate(const int sz, const int ext=1000) {return this->S->Allocate(sz,ext);};
  void Initialize() {this->S->Initialize();};
  char *GetClassName() {return "vtkBitmap";};

  // vtkScalar interface
  vtkScalars *MakeObject(int sze, int ext=1000);
  int GetNumberOfScalars() {return (this->S->GetMaxId()+1);};
  void Squeeze() {this->S->Squeeze();};

  // miscellaneous
  vtkBitmap &operator=(const vtkBitmap& fs);
  void operator+=(const vtkBitmap& fs) {*(this->S) += *(fs.S);};
  void Reset() {this->S->Reset();};
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
  vtkBitArray *S;
};

inline void vtkBitmap::SetNumberOfColors(int number)
{
  this->S->SetNumberOfValues(number);
}

// Description:
// Get pointer to byte containing bit in question. You will have to decompose
// byte to obtain appropriate bit value.
inline unsigned char *vtkBitmap::GetPtr(const int id)
{
  return this->S->GetPtr(id);
}

// Description:
// Get pointer to data. Useful for direct writes into object. MaxId is bumped
// by number (and memory allocated if necessary). Id is the location you 
// wish to write into; number is the number of rgba colors to write.
inline unsigned char *vtkBitmap::WritePtr(const int id, const int number)
{
  return this->S->WritePtr(id,number);
}

#endif
