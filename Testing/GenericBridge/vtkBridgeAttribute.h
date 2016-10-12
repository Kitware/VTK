/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBridgeAttribute.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkBridgeAttribute
 * @brief   Implementation of vtkGenericAttribute.
 *
 * It is just an example that show how to implement the Generic. It is also
 * used for testing and evaluating the Generic.
 * @sa
 * vtkGenericAttribute, vtkBridgeDataSet
*/

#ifndef vtkBridgeAttribute_h
#define vtkBridgeAttribute_h

#include "vtkBridgeExport.h" //for module export macro
#include "vtkGenericAttribute.h"

class vtkPointData;
class vtkCellData;
class vtkDataSetAttributes;

class VTKTESTINGGENERICBRIDGE_EXPORT vtkBridgeAttribute : public vtkGenericAttribute
{
 public:
  static vtkBridgeAttribute *New();
  vtkTypeMacro(vtkBridgeAttribute,vtkGenericAttribute);
  void PrintSelf(ostream& os, vtkIndent indent);

  /**
   * Name of the attribute. (e.g. "velocity")
   * \post result_may_not_exist: result!=0 || result==0
   */
  const char *GetName();

  /**
   * Dimension of the attribute. (1 for scalar, 3 for velocity)
   * \post positive_result: result>=0
   */
  int GetNumberOfComponents();

  /**
   * Is the attribute centered either on points, cells or boundaries?
   * \post valid_result: (result==vtkCenteringPoints) ||
   * (result==vtkCenteringCells) || (result==vtkCenteringBoundaries)
   */
  int GetCentering();

  /**
   * Type of the attribute: scalar, vector, normal, texture coordinate, tensor
   * \post valid_result: (result==vtkDataSetAttributes::SCALARS)
   * ||(result==vtkDataSetAttributes::VECTORS)
   * ||(result==vtkDataSetAttributes::NORMALS)
   * ||(result==vtkDataSetAttributes::TCOORDS)
   * ||(result==vtkDataSetAttributes::TENSORS)
   */
  int GetType();

  /**
   * Type of the components of the attribute: int, float, double
   * \post valid_result: (result==VTK_BIT)           ||(result==VTK_CHAR)
   * ||(result==VTK_UNSIGNED_CHAR) ||(result==VTK_SHORT)
   * ||(result==VTK_UNSIGNED_SHORT)||(result==VTK_INT)
   * ||(result==VTK_UNSIGNED_INT)  ||(result==VTK_LONG)
   * ||(result==VTK_UNSIGNED_LONG) ||(result==VTK_FLOAT)
   * ||(result==VTK_DOUBLE)        ||(result==VTK_ID_TYPE)
   */
  int GetComponentType();

  /**
   * Number of tuples.
   * \post valid_result: result>=0
   */
  vtkIdType GetSize();

  /**
   * Size in kibibytes (1024 bytes) taken by the attribute.
   */
  unsigned long GetActualMemorySize();

  /**
   * Range of the attribute component `component'. It returns double, even if
   * GetType()==VTK_INT.
   * NOT THREAD SAFE
   * \pre valid_component: (component>=0)&&(component<GetNumberOfComponents())
   * \post result_exists: result!=0
   */
  double *GetRange(int component);

  /**
   * Range of the attribute component `component'.
   * THREAD SAFE
   * \pre valid_component: (component>=0)&&(component<GetNumberOfComponents())
   */
  void GetRange(int component,
                double range[2]);

  /**
   * Return the maximum euclidean norm for the tuples.
   * \post positive_result: result>=0
   */
  double GetMaxNorm();

  /**
   * Attribute at all points of cell `c'.
   * \pre c_exists: c!=0
   * \pre c_valid: !c->IsAtEnd()
   * \post result_exists: result!=0
   * \post valid_result: sizeof(result)==GetNumberOfComponents()*c->GetCell()->GetNumberOfPoints()
   */
  virtual double *GetTuple(vtkGenericAdaptorCell *c);

  /**
   * Put attribute at all points of cell `c' in `tuple'.
   * \pre c_exists: c!=0
   * \pre c_valid: !c->IsAtEnd()
   * \pre tuple_exists: tuple!=0
   * \pre valid_tuple: sizeof(tuple)>=GetNumberOfComponents()*c->GetCell()->GetNumberOfPoints()
   */
  virtual void GetTuple(vtkGenericAdaptorCell *c, double *tuple);

  /**
   * Attribute at all points of cell `c'.
   * \pre c_exists: c!=0
   * \pre c_valid: !c->IsAtEnd()
   * \post result_exists: result!=0
   * \post valid_result: sizeof(result)==GetNumberOfComponents()*c->GetCell()->GetNumberOfPoints()
   */
  double *GetTuple(vtkGenericCellIterator *c);

  /**
   * Put attribute at all points of cell `c' in `tuple'.
   * \pre c_exists: c!=0
   * \pre c_valid: !c->IsAtEnd()
   * \pre tuple_exists: tuple!=0
   * \pre valid_tuple: sizeof(tuple)>=GetNumberOfComponents()*c->GetCell()->GetNumberOfPoints()
   */
  void GetTuple(vtkGenericCellIterator *c, double *tuple);

  /**
   * Value of the attribute at position `p'.
   * \pre p_exists: p!=0
   * \pre p_valid: !p->IsAtEnd()
   * \post result_exists: result!=0
   * \post valid_result_size: sizeof(result)==GetNumberOfComponents()
   */
  double *GetTuple(vtkGenericPointIterator *p);

  /**
   * Put the value of the attribute at position `p' into `tuple'.
   * \pre p_exists: p!=0
   * \pre p_valid: !p->IsAtEnd()
   * \pre tuple_exists: tuple!=0
   * \pre valid_tuple_size: sizeof(tuple)>=GetNumberOfComponents()
   */
  void GetTuple(vtkGenericPointIterator *p, double *tuple);

  /**
   * Put component `i' of the attribute at all points of cell `c' in `values'.
   * \pre valid_component: (i>=0) && (i<GetNumberOfComponents())
   * \pre c_exists: c!=0
   * \pre c_valid: !c->IsAtEnd()
   * \pre values_exist: values!=0
   * \pre valid_values: sizeof(values)>=c->GetCell()->GetNumberOfPoints()
   */
  void GetComponent(int i,vtkGenericCellIterator *c, double *values);

  /**
   * Value of the component `i' of the attribute at position `p'.
   * \pre valid_component: (i>=0) && (i<GetNumberOfComponents())
   * \pre p_exists: p!=0
   * \pre p_valid: !p->IsAtEnd()
   */
  double GetComponent(int i,vtkGenericPointIterator *p);

  /**
   * Recursive duplication of `other' in `this'.
   * \pre other_exists: other!=0
   * \pre not_self: other!=this
   */
  void DeepCopy(vtkGenericAttribute *other);

  /**
   * Update `this' using fields of `other'.
   * \pre other_exists: other!=0
   * \pre not_self: other!=this
   */
  void ShallowCopy(vtkGenericAttribute *other);

  /**
   * Set the current attribute to be centered on points with attribute `i' of
   * `d'.
   * \pre d_exists: d!=0
   * \pre valid_range: (i>=0) && (i<d->GetNumberOfArrays())
   */
  void InitWithPointData(vtkPointData *d,
                         int i);

  /**
   * Set the current attribute to be centered on cells with attribute `i' of
   * `d'.
   * \pre d_exists: d!=0
   * \pre valid_range: (i>=0) && (i<d->GetNumberOfArrays())
   */
  void InitWithCellData(vtkCellData *d,
                        int i);

protected:
  /**
   * Default constructor: empty attribute, not valid
   */
  vtkBridgeAttribute();
  /**
   * Destructor.
   */
  virtual ~vtkBridgeAttribute();

  /**
   * If size>InternalTupleCapacity, allocate enough memory.
   * \pre positive_size: size>0
   */
  void AllocateInternalTuple(int size);

  friend class vtkBridgeCell;

  // only one of them is non-null at a time.
  vtkPointData *Pd;
  vtkCellData *Cd;
  vtkDataSetAttributes *Data; // always not-null, equal to either on Pd or Cd
  int AttributeNumber;

  double *InternalTuple; // used by vtkBridgeCell
  int InternalTupleCapacity;

private:
  vtkBridgeAttribute(const vtkBridgeAttribute&) VTK_DELETE_FUNCTION;
  void operator=(const vtkBridgeAttribute&) VTK_DELETE_FUNCTION;
};

#endif
