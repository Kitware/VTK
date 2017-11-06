/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInformationQuadratureSchemeDefinitionVectorKey.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkInformationQuadratureSchemeDefinitionVectorKey
 * @brief   Key for vtkQuadratureSchemeDefinition vector values.
 *
 * vtkInformationQuadratureSchemeDefinitionVectorKey is used to represent keys for double
 * vector values in vtkInformation.h. NOTE the interface in this key differs
 * from that in other similar keys because of our internal use of smart
 * pointers.
*/

#ifndef vtkInformationQuadratureSchemeDefinitionVectorKey_h
#define vtkInformationQuadratureSchemeDefinitionVectorKey_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkInformationKey.h"
#include "vtkCommonInformationKeyManager.h" // Manage instances of this type.

class vtkInformationQuadratureSchemeDefinitionVectorValue;
class vtkXMLDataElement;
class vtkQuadratureSchemeDefinition;

class VTKCOMMONDATAMODEL_EXPORT vtkInformationQuadratureSchemeDefinitionVectorKey : public vtkInformationKey
{
public:
  vtkTypeMacro(vtkInformationQuadratureSchemeDefinitionVectorKey,vtkInformationKey);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  //@{
  /**
   * The name of the static instance and the class in which
   * it is defined(location) should be passed to the constructor.
   */
  vtkInformationQuadratureSchemeDefinitionVectorKey(
          const char* name,
          const char* location);
  //
  ~vtkInformationQuadratureSchemeDefinitionVectorKey() override;
  //@}

  /**
   * Clear the vector.
   */
  void Clear(vtkInformation* info);
  /**
   * Resize (extend) the vector to hold n objects. Any new elements
   * created will be null initialized.
   */
  void Resize(vtkInformation* info, int n);
  /**
   * Get the vector's length.
   */
  int Size(vtkInformation* info);
  int Length(vtkInformation* info){ return this->Size(info); }
  /**
   * Put the value on the back of the vector, with reference counting.
   */
  void Append(vtkInformation* info, vtkQuadratureSchemeDefinition *value);
  /**
   * Set element i of the vector to value. Resizes the vector
   * if needed.
   */
  void Set(vtkInformation* info, vtkQuadratureSchemeDefinition *value, int i);
  /**
   * Copy n values from the range in source defined by [from  from+n-1]
   * into the range in this vector defined by [to to+n-1]. Resizes
   * the vector if needed.
   */
  void SetRange(vtkInformation* info,
                vtkQuadratureSchemeDefinition **source,
                int from,
                int to,
                int n);

  /**
   * Copy n values from the range in this vector defined by [from  from+n-1]
   * into the range in the destination vector defined by [to to+n-1]. Up
   * to you to make sure the destination is big enough.
   */
  void GetRange(vtkInformation *info,
                vtkQuadratureSchemeDefinition **dest,
                int from,
                int to,
                int n);

  /**
   * Get the vtkQuadratureSchemeDefinition at a specific location in the vector.
   */
  vtkQuadratureSchemeDefinition *Get(vtkInformation* info, int idx);

  // _escription:
  // Get a pointer to the first vtkQuadratureSchemeDefinition in the vector. We are
  // uysing a vector of smart pointers so this is not easy to
  // implement.
  // vtkQuadratureSchemeDefinition **Get(vtkInformation* info);

  //@{
  /**
   * Copy the entry associated with this key from one information
   * object to another.  If there is no entry in the first information
   * object for this key, the value is removed from the second.
   */
  void ShallowCopy(vtkInformation* from, vtkInformation* to) override;
  void DeepCopy(vtkInformation* from, vtkInformation* to) override;
  //@}

  /**
   * Print the key's value in an information object to a stream.
   */
  void Print(ostream& os, vtkInformation* info) override;

  // note: I had wanted to make the following interface in vtkInformationKey
  // with a default implementation that did nothing. but we decided that
  // vtkInformationKey class is too important a class to add such an interface
  // without a thorough design review. we don't have budget for such a review.

  /**
   * Generate an XML representation of the object. Each
   * key/value pair will be nested in the resulting XML hierarchy.
   * The element passed in is assumed to be empty.
   */
  int SaveState(vtkInformation *info, vtkXMLDataElement *element);
  /**
   * Load key/value pairs from an XML state representation created
   * with SaveState. Duplicate keys will generate a fatal error.
   */
  int RestoreState(vtkInformation *info, vtkXMLDataElement *element);

private:
  /**
   * Used to create the underlying vector that will be associated
   * with this key.
   */
  void CreateQuadratureSchemeDefinition();
  /**
   * Get the vector associated with this key, if there is
   * none then associate a new vector with this key and return
   * that.
   */
  vtkInformationQuadratureSchemeDefinitionVectorValue *GetQuadratureSchemeDefinitionVector(vtkInformation *info);

  //
  vtkInformationQuadratureSchemeDefinitionVectorKey(const vtkInformationQuadratureSchemeDefinitionVectorKey&) = delete;
  void operator=(const vtkInformationQuadratureSchemeDefinitionVectorKey&) = delete;
};

#endif
