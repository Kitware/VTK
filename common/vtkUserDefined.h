/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkUserDefined.h
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
// .NAME vtkUserDefined - interface to user defined data
// .SECTION Description
// vtkUserDefined provides an abstract interface to user defined data. User 
// defined data are manipulated using void* pointers. These pointers are
// accessed via point id, so information can be represented on a per vertex 
// basis.

#ifndef __vtkUserDefined_h
#define __vtkUserDefined_h

#include "vtkRefCount.h"
#include "vtkVoidArray.h"
#include "vtkIdList.h"

class VTK_EXPORT vtkUserDefined : public vtkRefCount 
{
public:
  vtkUserDefined() {};
  vtkUserDefined(const vtkUserDefined& ud) {this->UD = ud.UD;};
  vtkUserDefined(const int sz, const int ext=1000):UD(sz,ext){};
  int Allocate(const int sz, const int ext=1000) {return this->UD.Allocate(sz,ext);};
  void Initialize() {this->UD.Initialize();};
  static vtkUserDefined *New() {return new vtkUserDefined;};
  char *GetClassName() {return "vtkUserDefined";};
  void PrintSelf(ostream& os, vtkIndent indent);

  vtkUserDefined *MakeObject(int sze, int ext=1000);
  int GetNumberOfUserDefined() {return (this->UD.GetMaxId()+1);};
  void Squeeze() {this->UD.Squeeze();};
  void* GetUserDefined(int i) {return this->UD.GetValue(i);};
  void SetNumberOfUserDefined(int number);
  void SetUserDefined(int i, void *ud) {this->UD.SetValue(i,ud);};
  void InsertUserDefined(int i, void *ud) {UD.InsertValue(i,ud);};
  int InsertNextUserDefined(void *ud) {return UD.InsertNextValue(ud);};
  void GetUserDefined(vtkIdList& ptId, vtkUserDefined& ud);

  vtkUserDefined &operator=(const vtkUserDefined& ud);
  void operator+=(const vtkUserDefined& ud) {this->UD += ud.UD;};
  void Reset() {this->UD.Reset();};

  // Description:
  // Interpolate user defined data. Method must be supplied by user. Return
  // value must be non-static void* pointer to data.
  virtual void* Interpolate(float *weights) {if (weights) return NULL; return NULL;};

protected:
  vtkVoidArray UD;
};

inline void vtkUserDefined::SetNumberOfUserDefined(int number)
{
  this->UD.SetNumberOfValues(number);
}

#endif
