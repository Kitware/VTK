/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMaskFields.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
// .NAME vtkMaskFields - Allow control of which fields get passed
// to the output
// .SECTION Description
// vtkMaskFields is used to mark which fields in the input dataset
// get copied to the output.  The output will contain only those fields
// marked as on by the filter.

// .SECTION See Also
// vtkFieldData vtkDataSet vtkDataObjectToDataSetFilter
// vtkDataSetAttributes vtkDataArray vtkRearrangeFields
// vtkSplitField vtkMergeFields vtkAssignAttribute

#ifndef __vtkMaskFields_h
#define __vtkMaskFields_h

#include "vtkDataSetToDataSetFilter.h"

class vtkDataSet;

class VTK_GRAPHICS_EXPORT vtkMaskFields : public vtkDataSetToDataSetFilter
{
public:
  vtkTypeRevisionMacro(vtkMaskFields,vtkDataSetToDataSetFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create a new vtkMaskFields.
  static vtkMaskFields *New();

  // Description:
  // Turn on/off the copying of the field or specified by name.
  // During the copying/passing, the following rules are followed for each
  // array:
  // 1. If the copy flag for an array is set (on or off), it is applied
  //    This overrides rule 2.
  // 2. If CopyAllOn is set, copy the array.
  //    If CopyAllOff is set, do not copy the array
  // A field name and a location must be specified. For example:
  // @verbatim
  // maskFields->CopyFieldOff(vtkMaskFields::CELL_DATA, "foo");
  // @endverbatim
  // causes the field "foo" on the input cell data to not get copied
  // to the output.
  void CopyFieldOn(int fieldLocation, const char* name) { this->CopyFieldOnOff(fieldLocation, name, 1); }
  void CopyFieldOff(int fieldLocation, const char* name) { this->CopyFieldOnOff(fieldLocation, name, 0); }


  // Description:
  // Turn on/off the copying of the attribute or specified by vtkDataSetAttributes:AttributeTypes.
  // During the copying/passing, the following rules are followed for each
  // array:
  // 1. If the copy flag for an array is set (on or off), it is applied
  //    This overrides rule 2.
  // 2. If CopyAllOn is set, copy the array.
  //    If CopyAllOff is set, do not copy the array
  // An attribute type and a location must be specified. For example:
  // @verbatim
  // maskFields->CopyAttributeOff(vtkMaskFields::POINT_DATA, vtkDataSetAttributes::SCALARS);
  // @endverbatim
  // causes the scalars on the input point data to not get copied
  // to the output.
  void CopyAttributeOn(int attributeLocation, int attributeType) { this->CopyAttributeOnOff(attributeLocation, attributeType, 1); }
  void CopyAttributeOff(int attributeLocation, int attributeType) { this->CopyAttributeOnOff(attributeLocation, attributeType, 0); }

  // Description:
  // Convenience methods which operate on all field data or 
  // attribute data.  More specific than CopyAllOn or CopyAllOff
  void CopyFieldsOff() { this->CopyFields = 0; }
  void CopyAttributesOff() { this->CopyAttributes = 0; }

  void CopyFieldsOn() { this->CopyFields = 1; }
  void CopyAttributesOn() { this->CopyAttributes = 1; }

  // Description:
  // Turn on copying of all data.
  // During the copying/passing, the following rules are followed for each
  // array:
  // 1. If the copy flag for an array is set (on or off), it is applied
  //    This overrides rule 2.
  // 2. If CopyAllOn is set, copy the array.
  //    If CopyAllOff is set, do not copy the array
  virtual void CopyAllOn();

  // Description:
  // Turn off copying of all data.
  // During the copying/passing, the following rules are followed for each
  // array:
  // 1. If the copy flag for an array is set (on or off), it is applied
  //    This overrides rule 2.
  // 2. If CopyAllOn is set, copy the array.
  //    If CopyAllOff is set, do not copy the array
  virtual void CopyAllOff();

  vtkMaskFields();
  virtual ~vtkMaskFields();

//BTX
  enum FieldLocation
    {
      OBJECT_DATA=0,
      POINT_DATA=1,
      CELL_DATA=2
    };
//ETX

protected:

  void Execute();

//BTX
  struct CopyFieldFlag
  {
    char* Name;
    int Type;
    int Location;
    int IsCopied;
  };
//ETX

  CopyFieldFlag* CopyFieldFlags; // the names of fields not to be copied
  int NumberOfFieldFlags; // the number of fields not to be copied
  void CopyFieldOnOff(int fieldLocation, const char* name, int onOff);
  void CopyAttributeOnOff(int attributeLocation, int attributeType, int onOff);
  void ClearFieldFlags();
  int FindFlag(const char* field, int location);
  int FindFlag(int arrayType, int location);
  int GetFlag(const char* field, int location);
  int GetFlag(int arrayType, int location);

  int CopyFields;
  int CopyAttributes;

private:
  vtkMaskFields(const vtkMaskFields&);  // Not implemented.
  void operator=(const vtkMaskFields&);  // Not implemented.
};

#endif


