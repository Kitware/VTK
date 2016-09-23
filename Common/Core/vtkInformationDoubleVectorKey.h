/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInformationDoubleVectorKey.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkInformationDoubleVectorKey
 * @brief   Key for double vector values.
 *
 * vtkInformationDoubleVectorKey is used to represent keys for double
 * vector values in vtkInformation.h
*/

#ifndef vtkInformationDoubleVectorKey_h
#define vtkInformationDoubleVectorKey_h

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkInformationKey.h"

#include "vtkCommonInformationKeyManager.h" // Manage instances of this type.

class VTKCOMMONCORE_EXPORT vtkInformationDoubleVectorKey : public vtkInformationKey
{
public:
  vtkTypeMacro(vtkInformationDoubleVectorKey,vtkInformationKey);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  vtkInformationDoubleVectorKey(const char* name, const char* location,
                                 int length=-1);
  ~vtkInformationDoubleVectorKey() VTK_OVERRIDE;

  /**
   * This method simply returns a new vtkInformationDoubleVectorKey, given a
   * name, a location and a required length. This method is provided for
   * wrappers. Use the constructor directly from C++ instead.
   */
  static vtkInformationDoubleVectorKey* MakeKey(const char* name, const char* location,
    int length=-1)
  {
    return new vtkInformationDoubleVectorKey(name, location, length);
  }

  //@{
  /**
   * Get/Set the value associated with this key in the given
   * information object.
   */
  void Append(vtkInformation* info, double value);
  void Set(vtkInformation* info, const double* value, int length);
  double* Get(vtkInformation* info);
  double  Get(vtkInformation* info, int idx);
  void Get(vtkInformation* info, double* value);
  int Length(vtkInformation* info);
  //@}

  /**
   * Copy the entry associated with this key from one information
   * object to another.  If there is no entry in the first information
   * object for this key, the value is removed from the second.
   */
  void ShallowCopy(vtkInformation* from, vtkInformation* to) VTK_OVERRIDE;

  /**
   * Print the key's value in an information object to a stream.
   */
  void Print(ostream& os, vtkInformation* info) VTK_OVERRIDE;

protected:
  // The required length of the vector value (-1 is no restriction).
  int RequiredLength;

private:
  vtkInformationDoubleVectorKey(const vtkInformationDoubleVectorKey&) VTK_DELETE_FUNCTION;
  void operator=(const vtkInformationDoubleVectorKey&) VTK_DELETE_FUNCTION;
};

#endif
