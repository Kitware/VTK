/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataObject.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

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
// .NAME vtkDataObject - general representation of visualization data
// .SECTION Description
// vtkDataObject is an general representation of visualization data. It serves
// to encapsulate instance variables and methods for visualization network 
// execution, as well as representing data consisting of a field,
//
// vtkDataObjects are used to represent arbitrary repositories of data via the
// vtkFieldData instance variable. These data must be eventually mapped into a
// concrete subclass of vtkDataSet before they can actually be displayed.
// .SECTION See Also
// vtkDataSet vtkFieldData vtkFieldDataSource vtkFieldDataFilter vtkFieldDataMapper

#ifndef __vtkDataObject_h
#define __vtkDataObject_h

#include "vtkObject.h"
#include "vtkFieldData.h"

class vtkSource;

class VTK_EXPORT vtkDataObject : public vtkObject
{
public:
  vtkDataObject();
  ~vtkDataObject();
  static vtkDataObject *New() {return new vtkDataObject;};
  const char *GetClassName() {return "vtkDataObject";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create concrete instance of this data object.
  virtual vtkDataObject *MakeObject() {return new vtkDataObject;};

  // Description:
  // Specify the source object creating this data object.
  vtkSetObjectMacro(Source,vtkSource);
  vtkGetObjectMacro(Source,vtkSource);
  
  // Description:
  // Provides opportunity for the data object to insure internal consistency 
  // before access. Also causes owning source/filter (if any) to update itself.
  virtual void Update();

  // Restore data object to initial state,
  virtual void Initialize();

  // Description:
  // Force the data object to update itself no matter what.
  virtual void ForceUpdate();

  // Description:
  // Release data back to system to conserve memory resource. Used during
  // visualization network execution.
  void ReleaseData();

  // Description:
  // Return flag indicating whether data should be released after use  
  // by a filter.
  int ShouldIReleaseData();

  // Description:
  // Set/Get the DataReleased ivar.
  vtkSetMacro(DataReleased,int);
  vtkGetMacro(DataReleased,int);

  // Description:
  // Turn on/off flag to control whether this object's data is released
  // after being used by a filter.
  vtkSetMacro(ReleaseDataFlag,int);
  vtkGetMacro(ReleaseDataFlag,int);
  vtkBooleanMacro(ReleaseDataFlag,int);

  // Description:
  // Turn on/off flag to control whether every object releases its data
  // after being used by a filter.
  static void SetGlobalReleaseDataFlag(int val);
  void GlobalReleaseDataFlagOn() {this->SetGlobalReleaseDataFlag(1);};
  void GlobalReleaseDataFlagOff() {this->SetGlobalReleaseDataFlag(0);};
  static int GetGlobalReleaseDataFlag();

  // return pointer to this data object's generic field data
  vtkFieldData *GetFieldData() {return &this->FieldData;};

protected:
  vtkSource *Source;
  vtkFieldData FieldData; //General field data associated with data object
  
  int DataReleased; //keep track of data release during network execution
  int ReleaseDataFlag; //data will release after use by a filter
};

#endif
