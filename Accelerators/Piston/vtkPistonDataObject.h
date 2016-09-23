/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPistonDataObject.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPistonDataObject
 * @brief   A GPU resident data set.
 *
 *
 * vtkPistonDataObject is a basic data structure for storing datasets on
 * GPU. This class provides the infrastructure for the VTK pipeline to
 * work with the data as it does the rest of the vtkDataObjects.
 * The GPU side structures are managed through the internal
 * vtkPistonReference instance to keep the GPU/CPU code conceptually
 * distinct.
 *
 * @sa
 * vtkPistonReference
*/

#ifndef vtkPistonDataObject_h
#define vtkPistonDataObject_h

#include "vtkAcceleratorsPistonModule.h" // For export macro
#include "vtkDataObject.h"

class vtkInformation;
class vtkInformationVector;
class vtkPistonReference;
class vtkTimeStamp;

class VTKACCELERATORSPISTON_EXPORT vtkPistonDataObject : public vtkDataObject
{
public:
  static vtkPistonDataObject* New();
  vtkTypeMacro(vtkPistonDataObject, vtkDataObject);
  void PrintSelf(ostream &os, vtkIndent indent);

  /**
   * From vtkType.h, a handle on what type of vtkDataObject this is.
   */
  int GetDataObjectType() {return VTK_PISTON_DATA_OBJECT;}

  /**
   * A convenience handle to get type of what is stored in the reference.
   */
  int GetReferredType();

  /**
   * A convenience handle to get whatever is actually stored in the reference.
   */
  void * GetReferredData();

  /**
   * GPU level representation and storage this manages.
   */
  vtkPistonReference *GetReference() { return this->Reference; };

  //@{
  /**
   * Shallow/deep copy the data from src into this object.
   */
  virtual void ShallowCopy(vtkDataObject* src);
  virtual void DeepCopy(vtkDataObject* src);
  //@}

  /**
   * Compute the data bounding box.
   */
  virtual void ComputeBounds();

  //@{
  /**
   * Return a pointer to the geometry bounding box in the form
   * (xmin,xmax, ymin,ymax, zmin,zmax).
   */
  double *GetBounds();
  void GetBounds(double bounds[6]);
  void SetBounds(const double bounds[6]);
  //@}

  //@{
  double *GetOrigin();
  void GetOrigin(double origin[3]);
  void SetOrigin(const double origin[3]);
  //@}

  //@{
  double *GetSpacing();
  void GetSpacing(double spacing[3]);
  void SetSpacing(double spacing[3]);
  //@}

  //@{
  /**
   * Get scalars array name
   */
  vtkGetStringMacro(ScalarsArrayName);
  // Set scalars array name
  vtkSetStringMacro(ScalarsArrayName);
  //@}

  //@{
  /**
   * Get scalars range
   * \NOTE: For now only one scalar is supported in Piston
   */
  double* GetScalarsRange();
  void GetScalarsRange(double range[2]);
  //@}
  /**
   * Set scalars range.
   */
  void SetScalarsRange(double range[2]);

  //@{
  /**
   * Retrieve an instance of this class from an information object.
   */
  static vtkPistonDataObject* GetData(vtkInformation* info);
  static vtkPistonDataObject* GetData(vtkInformationVector* v, int i=0);
  //@}

protected:
  vtkPistonDataObject();
  ~vtkPistonDataObject();

  vtkPistonReference *Reference;
  bool OwnReference;
  double Bounds[6];
  double Origin[3];
  double Spacing[3];

  char *ScalarsArrayName;
  double ScalarsRange[2];
  vtkTimeStamp ComputeTime; // Time at which bounds, center, etc. computed

private:
  vtkPistonDataObject(const vtkPistonDataObject&) VTK_DELETE_FUNCTION;
  void operator=(const vtkPistonDataObject&) VTK_DELETE_FUNCTION;
};

#endif
