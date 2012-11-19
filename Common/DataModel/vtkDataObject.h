/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataObject.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkDataObject - general representation of visualization data
// .SECTION Description
// vtkDataObject is an general representation of visualization data. It serves
// to encapsulate instance variables and methods for visualization network
// execution, as well as representing data consisting of a field (i.e., just
// an unstructured pile of data). This is to be compared with a vtkDataSet,
// which is data with geometric and/or topological structure.
//
// vtkDataObjects are used to represent arbitrary repositories of data via the
// vtkFieldData instance variable. These data must be eventually mapped into a
// concrete subclass of vtkDataSet before they can actually be displayed.
//
// .SECTION See Also
// vtkDataSet vtkFieldData vtkDataObjectSource vtkDataObjectFilter
// vtkDataObjectMapper vtkDataObjectToDataSet
// vtkFieldDataToAttributeDataFilter

#ifndef __vtkDataObject_h
#define __vtkDataObject_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkObject.h"

class vtkAbstractArray;
class vtkDataSetAttributes;
class vtkFieldData;
class vtkInformation;
class vtkInformationDataObjectKey;
class vtkInformationDoubleKey;
class vtkInformationDoubleVectorKey;
class vtkInformationIntegerKey;
class vtkInformationIntegerPointerKey;
class vtkInformationIntegerVectorKey;
class vtkInformationStringKey;
class vtkInformationVector;
class vtkInformationInformationVectorKey;

#define VTK_PIECES_EXTENT   0
#define VTK_3D_EXTENT       1
#define VTK_TIME_EXTENT     2

class VTKCOMMONDATAMODEL_EXPORT vtkDataObject : public vtkObject
{
public:
  static vtkDataObject *New();

  vtkTypeMacro(vtkDataObject,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set/Get the information object associated with this data object.
  vtkGetObjectMacro(Information, vtkInformation);
  virtual void SetInformation(vtkInformation*);

  // Description:
  // Data objects are composite objects and need to check each part for MTime.
  // The information object also needs to be considered.
  unsigned long int GetMTime();

  // Description:
  // Restore data object to initial state,
  virtual void Initialize();

  // Description:
  // Release data back to system to conserve memory resource. Used during
  // visualization network execution.  Releasing this data does not make
  // down-stream data invalid, so it does not modify the MTime of this
  // data object.
  void ReleaseData();

  // Description:
  // Get the flag indicating the data has been released.
  vtkGetMacro(DataReleased,int);


  // Description:
  // Turn on/off flag to control whether every object releases its data
  // after being used by a filter.
  static void SetGlobalReleaseDataFlag(int val);
  void GlobalReleaseDataFlagOn() {this->SetGlobalReleaseDataFlag(1);};
  void GlobalReleaseDataFlagOff() {this->SetGlobalReleaseDataFlag(0);};
  static int GetGlobalReleaseDataFlag();

  // Description:
  // Assign or retrieve a general field data to this data object.
  virtual void SetFieldData(vtkFieldData*);
  vtkGetObjectMacro(FieldData,vtkFieldData);

  // Handle the source/data loop.
  virtual void Register(vtkObjectBase* o);
  virtual void UnRegister(vtkObjectBase* o);

  // Description:
  // Return class name of data type. This is one of VTK_STRUCTURED_GRID,
  // VTK_STRUCTURED_POINTS, VTK_UNSTRUCTURED_GRID, VTK_POLY_DATA, or
  // VTK_RECTILINEAR_GRID (see vtkSetGet.h for definitions).
  // THIS METHOD IS THREAD SAFE
  virtual int GetDataObjectType() {return VTK_DATA_OBJECT;}

  // Description:
  // Used by Threaded ports to determine if they should initiate an
  // asynchronous update (still in development).
  unsigned long GetUpdateTime();

  // Description:
  // Return the actual size of the data in kilobytes. This number
  // is valid only after the pipeline has updated. The memory size
  // returned is guaranteed to be greater than or equal to the
  // memory required to represent the data (e.g., extra space in
  // arrays, etc. are not included in the return value).
  virtual unsigned long GetActualMemorySize();

   // Description:
   // Copy information about this data object from the
   // pipeline information to its own Information.
  // Called right before the main execution pass..
  virtual void CopyInformationFromPipeline(vtkInformation* vtkNotUsed(info))
  {}

  // Description:
  // Return the information object within the input information object's
  // field data corresponding to the specified association
  // (FIELD_ASSOCIATION_POINTS or FIELD_ASSOCIATION_CELLS) and attribute
  // (SCALARS, VECTORS, NORMALS, TCOORDS, or TENSORS)
  static vtkInformation *GetActiveFieldInformation(vtkInformation *info,
    int fieldAssociation, int attributeType);

  // Description:
  // Return the information object within the input information object's
  // field data corresponding to the specified association
  // (FIELD_ASSOCIATION_POINTS or FIELD_ASSOCIATION_CELLS) and name.
  static vtkInformation *GetNamedFieldInformation(vtkInformation *info,
    int fieldAssociation, const char *name);

  // Description:
  // Remove the info associated with an array
  static void RemoveNamedFieldInformation(vtkInformation *info,
                                          int fieldAssociation,
                                          const char *name);

  // Description:
  // Set the named array to be the active field for the specified type
  // (SCALARS, VECTORS, NORMALS, TCOORDS, or TENSORS) and association
  // (FIELD_ASSOCIATION_POINTS or FIELD_ASSOCIATION_CELLS).  Returns the
  // active field information object and creates on entry if one not found.
  static vtkInformation *SetActiveAttribute(vtkInformation *info,
    int fieldAssociation, const char *attributeName, int attributeType);

  // Description:
  // Set the name, array type, number of components, and number of tuples
  // within the passed information object for the active attribute of type
  // attributeType (in specified association, FIELD_ASSOCIATION_POINTS or
  // FIELD_ASSOCIATION_CELLS).  If there is not an active attribute of the
  // specified type, an entry in the information object is created.  If
  // arrayType, numComponents, or numTuples equal to -1, or name=NULL the
  // value is not changed.
  static void SetActiveAttributeInfo(vtkInformation *info,
    int fieldAssociation, int attributeType, const char *name, int arrayType,
    int numComponents, int numTuples);

  // Description:
  // Convenience version of previous method for use (primarily) by the Imaging
  // filters. If arrayType or numComponents == -1, the value is not changed.
  static void SetPointDataActiveScalarInfo(vtkInformation *info,
    int arrayType, int numComponents);

  // Description:
  // This method is called by the source when it executes to generate data.
  // It is sort of the opposite of ReleaseData.
  // It sets the DataReleased flag to 0, and sets a new UpdateTime.
  void DataHasBeenGenerated();

  // Description:
  // make the output data ready for new data to be inserted. For most
  // objects we just call Initialize. But for vtkImageData we leave the old
  // data in case the memory can be reused.
  virtual void PrepareForNewData() {this->Initialize();};

  // Description:
  // Shallow and Deep copy.  These copy the data, but not any of the
  // pipeline connections.
  virtual void ShallowCopy(vtkDataObject *src);
  virtual void DeepCopy(vtkDataObject *src);

  // Description:
  // The ExtentType will be left as VTK_PIECES_EXTENT for data objects
  // such as vtkPolyData and vtkUnstructuredGrid. The ExtentType will be
  // changed to VTK_3D_EXTENT for data objects with 3D structure such as
  // vtkImageData (and its subclass vtkStructuredPoints), vtkRectilinearGrid,
  // and vtkStructuredGrid. The default is the have an extent in pieces,
  // with only one piece (no streaming possible).
  virtual int GetExtentType() { return VTK_PIECES_EXTENT; };

  // Description:
  // This method crops the data object (if necessary) so that the extent
  // matches the update extent.
  virtual void Crop(const int* updateExtent);

  //BTX
  // Description:
  // Possible values for the FIELD_ASSOCIATION information entry.
  enum FieldAssociations
  {
    FIELD_ASSOCIATION_POINTS,
    FIELD_ASSOCIATION_CELLS,
    FIELD_ASSOCIATION_NONE,
    FIELD_ASSOCIATION_POINTS_THEN_CELLS,
    FIELD_ASSOCIATION_VERTICES,
    FIELD_ASSOCIATION_EDGES,
    FIELD_ASSOCIATION_ROWS,
    NUMBER_OF_ASSOCIATIONS
  };
  //ETX

  //BTX
  // Description:
  // Possible attribute types.
  // POINT_THEN_CELL is provided for consistency with FieldAssociations.
  enum AttributeTypes
  {
    POINT,
    CELL,
    FIELD,
    POINT_THEN_CELL,
    VERTEX,
    EDGE,
    ROW,
    NUMBER_OF_ATTRIBUTE_TYPES
  };
  //ETX

  // Description:
  // Returns the attributes of the data object of the specified
  // attribute type. The type may be:
  // <ul>
  // <li>POINT  - Defined in vtkDataSet subclasses.
  // <li>CELL   - Defined in vtkDataSet subclasses.
  // <li>VERTEX - Defined in vtkGraph subclasses.
  // <li>EDGE   - Defined in vtkGraph subclasses.
  // <li>ROW    - Defined in vtkTable.
  // </ul>
  // The other attribute type, FIELD, will return NULL since
  // field data is stored as a vtkFieldData instance, not a
  // vtkDataSetAttributes instance. To retrieve field data, use
  // GetAttributesAsFieldData.
  virtual vtkDataSetAttributes* GetAttributes(int type);

  // Description:
  // Returns the attributes of the data object as a vtkFieldData.
  // This returns non-null values in all the same cases as GetAttributes,
  // in addition to the case of FIELD, which will return the field data
  // for any vtkDataObject subclass.
  virtual vtkFieldData* GetAttributesAsFieldData(int type);

  // Description:
  // Retrieves the attribute type that an array came from.
  // This is useful for obtaining which attribute type a input array
  // to an algorithm came from (retrieved from GetInputAbstractArrayToProcesss).
  virtual int GetAttributeTypeForArray(vtkAbstractArray* arr);

  // Description:
  // Get the number of elements for a specific attribute type (POINT, CELL, etc.).
  virtual vtkIdType GetNumberOfElements(int type);

  //BTX
  // Description:
  // Possible values for the FIELD_OPERATION information entry.
  enum FieldOperations
  {
    FIELD_OPERATION_PRESERVED,
    FIELD_OPERATION_REINTERPOLATED,
    FIELD_OPERATION_MODIFIED,
    FIELD_OPERATION_REMOVED
  };
  //ETX

  // Description:
  // Given an integer association type, this static method returns a string type
  // for the attribute (i.e. type = 0: returns "Points").
  static const char* GetAssociationTypeAsString(int associationType);

  // Description:
  // Given an integer association type, this static method returns a string type
  // for the attribute (i.e. type = 0: returns "Points").
  static int GetAssociationTypeFromString(const char* associationType);

  static vtkInformationStringKey* DATA_TYPE_NAME();
  static vtkInformationDataObjectKey* DATA_OBJECT();
  static vtkInformationIntegerKey* DATA_EXTENT_TYPE();
  static vtkInformationIntegerPointerKey* DATA_EXTENT();
  static vtkInformationIntegerKey* DATA_PIECE_NUMBER();
  static vtkInformationIntegerKey* DATA_NUMBER_OF_PIECES();
  static vtkInformationIntegerKey* DATA_NUMBER_OF_GHOST_LEVELS();
  static vtkInformationDoubleKey* DATA_RESOLUTION();
  static vtkInformationDoubleKey* DATA_TIME_STEP();
  static vtkInformationInformationVectorKey* POINT_DATA_VECTOR();
  static vtkInformationInformationVectorKey* CELL_DATA_VECTOR();
  static vtkInformationInformationVectorKey* VERTEX_DATA_VECTOR();
  static vtkInformationInformationVectorKey* EDGE_DATA_VECTOR();
  static vtkInformationIntegerKey* FIELD_ARRAY_TYPE();
  static vtkInformationIntegerKey* FIELD_ASSOCIATION();
  static vtkInformationIntegerKey* FIELD_ATTRIBUTE_TYPE();
  static vtkInformationIntegerKey* FIELD_ACTIVE_ATTRIBUTE();
  static vtkInformationIntegerKey* FIELD_NUMBER_OF_COMPONENTS();
  static vtkInformationIntegerKey* FIELD_NUMBER_OF_TUPLES();
  static vtkInformationIntegerKey* FIELD_OPERATION();
  static vtkInformationDoubleVectorKey* FIELD_RANGE();
  static vtkInformationDoubleVectorKey* PIECE_FIELD_RANGE();
  static vtkInformationStringKey* FIELD_ARRAY_NAME();
  static vtkInformationIntegerVectorKey* PIECE_EXTENT();
  static vtkInformationStringKey* FIELD_NAME();
  static vtkInformationDoubleVectorKey* ORIGIN();
  static vtkInformationDoubleVectorKey* SPACING();
  static vtkInformationIntegerKey* DATA_GEOMETRY_UNMODIFIED();
  static vtkInformationDoubleVectorKey* BOUNDING_BOX();

  // Key used to put SIL information in the output information by readers.
  static vtkInformationDataObjectKey* SIL();

  //BTX
  // Description:
  // Retrieve an instance of this class from an information object.
  static vtkDataObject* GetData(vtkInformation* info);
  static vtkDataObject* GetData(vtkInformationVector* v, int i=0);
  //ETX

protected:

  vtkDataObject();
  ~vtkDataObject();

  // General field data associated with data object
  vtkFieldData  *FieldData;

  // Keep track of data release during network execution
  int DataReleased;

  // When was this data last generated?
  vtkTimeStamp UpdateTime;

  virtual void ReportReferences(vtkGarbageCollector*);

  // Arbitrary extra information associated with this data object.
  vtkInformation* Information;

private:
  // Helper method for the ShallowCopy and DeepCopy methods.
  void InternalDataObjectCopy(vtkDataObject *src);

private:
  vtkDataObject(const vtkDataObject&);  // Not implemented.
  void operator=(const vtkDataObject&);  // Not implemented.
};

#endif

