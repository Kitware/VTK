/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGenericAttributeCollection.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkGenericAttributeCollection
 * @brief   a collection of attributes
 *
 * vtkGenericAttributeCollection is a class that collects attributes
 * (represented by vtkGenericAttribute).
*/

#ifndef vtkGenericAttributeCollection_h
#define vtkGenericAttributeCollection_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkObject.h"

class vtkGenericAttributeInternalVector;
class vtkIntInternalVector;
class vtkGenericAttribute;

class VTKCOMMONDATAMODEL_EXPORT vtkGenericAttributeCollection : public vtkObject
{
public:
  /**
   * Create an empty collection.
   */
  static vtkGenericAttributeCollection *New();

  //@{
  /**
   * Standard type definition and print methods for a VTK class.
   */
  vtkTypeMacro(vtkGenericAttributeCollection,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;
  //@}

  /**
   * Return the number of attributes (e.g., instances of vtkGenericAttribute)
   * in the collection.
   * \post positive_result: result>=0
   */
  int GetNumberOfAttributes();

  /**
   * Return the number of components. This is the sum of all components
   * found in all attributes.
   * \post positive_result: result>=0
   */
  int GetNumberOfComponents();

  /**
   * Return the number of components. This is the sum of all components
   * found in all point centered attributes.
   * \post positive_result: result>=0
   */
  int GetNumberOfPointCenteredComponents();

  /**
   * Maximum number of components encountered among all attributes.
   * \post positive_result: result>=0
   * \post valid_result: result<=GetNumberOfComponents()
   */
  int GetMaxNumberOfComponents();

  /**
   * Actual size of the data in kibibytes (1024 bytes); only valid after the pipeline has
   * updated. It is guaranteed to be greater than or equal to the memory
   * required to represent the data.
   */
  unsigned long GetActualMemorySize();

  /**
   * Indicate whether the collection contains any attributes.
   * \post definition: result==(GetNumberOfAttributes()==0)
   */
  int IsEmpty();

  /**
   * Return a pointer to the ith instance of vtkGenericAttribute.
   * \pre not_empty: !IsEmpty()
   * \pre valid_i: i>=0 && i<GetNumberOfAttributes()
   * \post result_exists: result!=0
   */
  vtkGenericAttribute *GetAttribute(int i);

  /**
   * Return the index of the attribute named `name'. Return the non-negative
   * index if found. Return -1 otherwise.
   * \pre name_exists: name!=0
   * \post valid_result: (result==-1) || (result>=0) && (result<=GetNumberOfAttributes())
   */
  int FindAttribute(const char *name);

  /**
   * Return the index of the first component of attribute `i' in an array of
   * format attrib0comp0 attrib0comp1 ... attrib4comp0 ...
   * \pre valid_i: i>=0 && i<GetNumberOfAttributes()
   * \pre is_point_centered: GetAttribute(i)->GetCentering()==vtkPointCentered
   */
  int GetAttributeIndex(int i);

  /**
   * Add the attribute `a' to the end of the collection.
   * \pre a_exists: a!=0
   * \post more_items: GetNumberOfAttributes()==old GetNumberOfAttributes()+1
   * \post a_is_set: GetAttribute(GetNumberOfAttributes()-1)==a
   */
  void InsertNextAttribute(vtkGenericAttribute *a);

  /**
   * Replace the attribute at index `i' by `a'.
   * \pre not_empty: !IsEmpty()
   * \pre a_exists: a!=0
   * \pre valid_i: i>=0 && i<GetNumberOfAttributes()
   * \post same_size: GetNumberOfAttributes()==old GetNumberOfAttributes()
   * \post item_is_set: GetAttribute(i)==a
   */
  void InsertAttribute(int i, vtkGenericAttribute *a);

  /**
   * Remove the attribute at `i'.
   * \pre not_empty: !IsEmpty()
   * \pre valid_i: i>=0 && i<GetNumberOfAttributes()
   * \post fewer_items: GetNumberOfAttributes()==old GetNumberOfAttributes()-1
   */
  void RemoveAttribute(int i);

  /**
   * Remove all attributes.
   * \post is_empty: GetNumberOfAttributes()==0
   */
  void Reset();

  /**
   * Copy, without reference counting, the other attribute array.
   * \pre other_exists: other!=0
   * \pre not_self: other!=this
   * \post same_size: GetNumberOfAttributes()==other->GetNumberOfAttributes()
   */
  void DeepCopy(vtkGenericAttributeCollection *other);

  /**
   * Copy, via reference counting, the other attribute array.
   * \pre other_exists: other!=0
   * \pre not_self: other!=this
   * \post same_size: GetNumberOfAttributes()==other->GetNumberOfAttributes()
   */
  void ShallowCopy(vtkGenericAttributeCollection *other);

  /**
   * vtkAttributeCollection is a composite object and needs to check each
   * member of its collection for modified time.
   */
  vtkMTimeType GetMTime() VTK_OVERRIDE;

  // *** ALL THE FOLLOWING METHODS SHOULD BE REMOVED WHEN when the
  // new pipeline update mechanism is checked in.
  // *** BEGIN

  //@{
  /**
   * Index of the attribute to be processed (not necessarily scalar).
   * \pre not_empty: !IsEmpty()
   * \post valid_result: result>=0 && result<GetNumberOfAttributes()
   */
  vtkGetMacro(ActiveAttribute, int);
  //@}

  //@{
  /**
   * Component of the active attribute to be processed. -1 means module.
   * \pre not_empty: GetNumberOfAttributes()>0
   * \post valid_result: result>=-1 &&
   * result<GetAttribute(GetActiveAttribute())->GetNumberOfComponents()
   */
  vtkGetMacro(ActiveComponent, int);
  //@}

  /**
   * Set the scalar attribute to be processed. -1 means module.
   * \pre not_empty: !IsEmpty()
   * \pre valid_attribute: attribute>=0 && attribute<GetNumberOfAttributes()
   * \pre valid_component: component>=-1 &&
   * component<GetAttribute(attribute)->GetNumberOfComponents()
   * \post is_set: GetActiveAttribute()==attribute &&
   * GetActiveComponent()==component
   */
  void SetActiveAttribute(int attribute, int component = 0);

  //@{
  /**
   * Number of attributes to interpolate.
   * \pre not_empty: !IsEmpty()
   * \post positive_result: result>=0
   */
  vtkGetMacro(NumberOfAttributesToInterpolate, int);
  //@}

  /**
   * Indices of attributes to interpolate.
   * \pre not_empty: !IsEmpty()
   * \post valid_result: GetNumberOfAttributesToInterpolate()>0
   */
  int *GetAttributesToInterpolate();

  /**
   * Does the array `attributes' of size `size' have `attribute'?
   * \pre positive_size: size>=0
   * \pre valid_attributes: size>0 implies attributes!=0
   */
  int HasAttribute(int size, int *attributes, int attribute);

  //@{
  /**
   * Set the attributes to interpolate.
   * \pre not_empty: !IsEmpty()
   * \pre positive_size: size>=0
   * \pre valid_attributes: size>0 implies attributes!=0
   * \pre valid_attributes_contents: attributes!=0 implies
   * !HasAttributes(size,attributes,GetActiveAttribute())
   * \post is_set: (GetNumberOfAttributesToInterpolate()==size)&&
   * (GetAttributesToInterpolate()==attributes)
   */
  void SetAttributesToInterpolate(int size, int *attributes);
  void SetAttributesToInterpolateToAll();
  //@}

protected:
  /**
   * Default constructor: empty collection.
   */
  vtkGenericAttributeCollection();

  /**
   * Destructor.
   */
  ~vtkGenericAttributeCollection() VTK_OVERRIDE;

  /**
   * STL vector for storing attributes
   */
  vtkGenericAttributeInternalVector* AttributeInternalVector;
  /**
   * STL vector for storing index of point centered attributes
   */
  vtkIntInternalVector *AttributeIndices;

  int ActiveAttribute;
  int ActiveComponent;
  int NumberOfAttributesToInterpolate;
  int AttributesToInterpolate[10];

  int NumberOfComponents; // cache
  int NumberOfPointCenteredComponents; // cache
  int MaxNumberOfComponents; // cache
  unsigned long ActualMemorySize; // cache
  vtkTimeStamp ComputeTime; // cache time stamp

  /**
   * Compute number of components, max number of components and actual
   * memory size.
   */
  void ComputeNumbers();

private:
  vtkGenericAttributeCollection(const vtkGenericAttributeCollection &) VTK_DELETE_FUNCTION;
  void operator=(const vtkGenericAttributeCollection &) VTK_DELETE_FUNCTION;
};
#endif
