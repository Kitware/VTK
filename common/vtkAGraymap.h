/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAGraymap.h
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
// .NAME vtkAGraymap - scalar data in intensity + alpha (grayscale + opacity) form
// .SECTION Description
// vtkAGraymap is a concrete implementation of vtkColorScalars. vtkAGraymap 
// represents scalars using one value for intensity (grayscale) and
// one value for alpha (opacity). The intensity and alpha values range 
// between (0,255) (i.e., an unsigned char value).
//
// If you use the method SetColor() (inherited from superclass vtkColorScalars)
// the rgba components are converted to intensity-alpha using the standard
// luminance equation Luminance = 0.30*red + 0.59*green + 0.11*blue.
// .SECTION See Also
// vtkGraymap vtkPixmap vtkAPixmap vtkBitmap

#ifndef __vtkAGraymap_h
#define __vtkAGraymap_h

#include "vtkColorScalars.h"
#include "vtkUnsignedCharArray.h"

class VTK_EXPORT vtkAGraymap : public vtkColorScalars 
{
public:
  vtkAGraymap();
  vtkAGraymap(const vtkAGraymap& fs);
  vtkAGraymap(const int sz, const int ext=1000);
  ~vtkAGraymap();
  
  int Allocate(const int sz, const int ext=1000) {return this->S->Allocate(2*sz,2*ext);};
  void Initialize() {this->S->Initialize();};
  static vtkAGraymap *New() {return new vtkAGraymap;};
  const char *GetClassName() {return "vtkAGraymap";};

  // vtkScalar interface
  vtkScalars *MakeObject(int sze, int ext=1000);
  int GetNumberOfScalars() {return (this->S->GetMaxId()+1)/2;};
  void Squeeze() {this->S->Squeeze();};
  int GetNumberOfValuesPerScalar() {return 2;};
  float GetScalar(int i);

  // miscellaneous
  vtkAGraymap &operator=(const vtkAGraymap& fs);
  void operator+=(const vtkAGraymap& fs) {*(this->S) += *(fs.S);};
  void Reset() {this->S->Reset();};
  unsigned char *GetPointer(const int id);
  unsigned char *WritePointer(const int id, const int number);

  // vtkColorScalar interface.
  unsigned char *GetColor(int id);
  void GetColor(int id, unsigned char rgba[4]);
  void SetNumberOfColors(int number);
  void SetColor(int id, unsigned char rgba[4]);
  void InsertColor(int id, unsigned char rgba[4]);
  int InsertNextColor(unsigned char rgba[4]);

  // methods specific to this class
  unsigned char *GetAGrayValue(int id);
  void GetAGrayValue(int id, unsigned char ga[2]);
  void SetAGrayValue(int id, unsigned char ga[2]);
  void InsertAGrayValue(int id, unsigned char ga[2]);
  int InsertNextAGrayValue(unsigned char ga[2]);

  // Used by vtkImageToStructuredPoints (Proper length array is up to user!)
  vtkSetReferenceCountedObjectMacro(S, vtkUnsignedCharArray);
  vtkGetObjectMacro(S, vtkUnsignedCharArray);

protected:
  vtkUnsignedCharArray *S;
};

// Description:
// Set a rgba color value at a particular array location. Does not do 
// range checking. Make sure you use SetNumberOfColors() to allocate 
// memory prior to using SetColor().
inline void vtkAGraymap::SetColor(int i, unsigned char rgba[4]) 
{
  float g = 0.30*rgba[0] + 0.59*rgba[1] + 0.11*rgba[2];
  g = (g > 255.0 ? 255.0 : g);

  i *= 2; 
  this->S->SetValue(i, (unsigned char)g);
  this->S->SetValue(i+1, rgba[3]); 
}

// Description:
// Insert a rgba color value at a particular array location. Does range 
// checking and will allocate additional memory if necessary.
inline void vtkAGraymap::InsertColor(int i, unsigned char rgba[4]) 
{
  float g = 0.30*rgba[0] + 0.59*rgba[1] + 0.11*rgba[2];
  g = (g > 255.0 ? 255.0 : g);

  this->S->InsertValue(2*i+1, rgba[3]);
  this->S->SetValue(2*i, (unsigned char)g);
}

// Description:
// Insert a rgba color value at the next available slot in the array. Will
// allocate memory if necessary.
inline int vtkAGraymap::InsertNextColor(unsigned char rgba[4]) 
{
  int id;
  float g = 0.30*rgba[0] + 0.59*rgba[1] + 0.11*rgba[2];
  g = (g > 255.0 ? 255.0 : g);

  id = this->S->InsertNextValue((unsigned char)g);
  this->S->InsertNextValue(rgba[3]);

  return id/2;
}

// Description:
// Get pointer to array of data starting at data position "id". Form of
// data is a list of repeated intensity/alpha pairs.
inline unsigned char *vtkAGraymap::GetPointer(const int id)
{
  return this->S->GetPointer(2*id);
}

// Description:
// Get pointer to data array. Useful for direct writes of data. MaxId is 
// bumped by number (and memory allocated if necessary). Id is the 
// location you wish to write into; number is the number of scalars to 
// write. 
inline unsigned char *vtkAGraymap::WritePointer(const int id, const int number)
{
  return this->S->WritePointer(2*id,2*number);
}

#endif
