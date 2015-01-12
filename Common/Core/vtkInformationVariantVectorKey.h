/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInformationVariantVectorKey.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkInformationVariantVectorKey - Key for variant vector values.
// .SECTION Description
// vtkInformationVariantVectorKey is used to represent keys for variant
// vector values in vtkInformation.h

#ifndef vtkInformationVariantVectorKey_h
#define vtkInformationVariantVectorKey_h

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkInformationKey.h"

#include "vtkCommonInformationKeyManager.h" // Manage instances of this type.

class vtkVariant;

class VTKCOMMONCORE_EXPORT vtkInformationVariantVectorKey : public vtkInformationKey
{
public:
  vtkTypeMacro(vtkInformationVariantVectorKey,vtkInformationKey);
  void PrintSelf(ostream& os, vtkIndent indent);

  vtkInformationVariantVectorKey(const char* name, const char* location,
                                 int length=-1);
  ~vtkInformationVariantVectorKey();

  // Description:
  // This method simply returns a new vtkInformationVariantVectorKey, given a
  // name, a location and a required length. This method is provided for
  // wrappers. Use the constructor directly from C++ instead.
  static vtkInformationVariantVectorKey* MakeKey(const char* name, const char* location,
    int length=-1)
    {
    return new vtkInformationVariantVectorKey(name, location, length);
    }

  // Description:
  // Get/Set the value associated with this key in the given
  // information object.
  void Append(vtkInformation* info, const vtkVariant& value);
  void Set(vtkInformation* info, const vtkVariant* value, int length);
  const vtkVariant* Get(vtkInformation* info) const;
  const vtkVariant& Get(vtkInformation* info, int idx) const;
  void Get(vtkInformation* info, vtkVariant* value) const;
  int Length(vtkInformation* info) const;

  // Description:
  // Copy the entry associated with this key from one information
  // object to another.  If there is no entry in the first information
  // object for this key, the value is removed from the second.
  virtual void ShallowCopy(vtkInformation* from, vtkInformation* to);

  // Description:
  // Print the key's value in an information object to a stream.
  virtual void Print(ostream& os, vtkInformation* info);

protected:
  // The required length of the vector value (-1 is no restriction).
  int RequiredLength;

private:
  vtkInformationVariantVectorKey(const vtkInformationVariantVectorKey&);  // Not implemented.
  void operator=(const vtkInformationVariantVectorKey&);  // Not implemented.
};

#endif
