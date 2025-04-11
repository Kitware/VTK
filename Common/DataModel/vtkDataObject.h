// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkDataObject
 * @brief   general representation of visualization data
 *
 * vtkDataObject is an general representation of visualization data. It serves
 * to encapsulate instance variables and methods for visualization network
 * execution, as well as representing data consisting of a field (i.e., just
 * an unstructured pile of data). This is to be compared with a vtkDataSet,
 * which is data with geometric and/or topological structure.
 *
 * vtkDataObjects are used to represent arbitrary repositories of data via the
 * vtkFieldData instance variable. These data must be eventually mapped into a
 * concrete subclass of vtkDataSet before they can actually be displayed.
 *
 * @sa
 * vtkDataSet vtkFieldData vtkDataObjectToDataSetFilter
 * vtkFieldDataToAttributeDataFilter
 */

#ifndef vtkDataObject_h
#define vtkDataObject_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkObject.h"
#include "vtkWrappingHints.h" // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
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
class vtkUnsignedCharArray;

#define VTK_PIECES_EXTENT 0
#define VTK_3D_EXTENT 1
#define VTK_TIME_EXTENT 2

class VTKCOMMONDATAMODEL_EXPORT VTK_MARSHALAUTO vtkDataObject : public vtkObject
{
public:
  static vtkDataObject* New();

  vtkTypeMacro(vtkDataObject, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Set/Get the information object associated with this data object.
   */
  vtkGetObjectMacro(Information, vtkInformation);
  virtual void SetInformation(vtkInformation*);
  ///@}

  /**
   * Data objects are composite objects and need to check each part for MTime.
   * The information object also needs to be considered.
   */
  vtkMTimeType GetMTime() override;

  /**
   * Restore data object to initial state,
   */
  virtual void Initialize();

  /**
   * Release data back to system to conserve memory resource. Used during
   * visualization network execution.  Releasing this data does not make
   * down-stream data invalid.
   */
  void ReleaseData();

  ///@{
  /**
   * Get the flag indicating the data has been released.
   */
  vtkGetMacro(DataReleased, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Turn on/off flag to control whether every object releases its data
   * after being used by a filter.
   */
  static void SetGlobalReleaseDataFlag(vtkTypeBool val);
  void GlobalReleaseDataFlagOn() { vtkDataObject::SetGlobalReleaseDataFlag(1); }
  void GlobalReleaseDataFlagOff() { vtkDataObject::SetGlobalReleaseDataFlag(0); }
  static vtkTypeBool GetGlobalReleaseDataFlag();
  ///@}

  ///@{
  /**
   * Assign or retrieve a general field data to this data object.
   */
  virtual void SetFieldData(vtkFieldData*);
  vtkGetObjectMacro(FieldData, vtkFieldData);
  ///@}

  /**
   * Return class name of data type. This is one of VTK_STRUCTURED_GRID,
   * VTK_STRUCTURED_POINTS, VTK_UNSTRUCTURED_GRID, VTK_POLY_DATA, or
   * VTK_RECTILINEAR_GRID (see vtkSetGet.h for definitions).
   * THIS METHOD IS THREAD SAFE
   */
  virtual int GetDataObjectType() VTK_FUTURE_CONST { return VTK_DATA_OBJECT; }

  /**
   * Used by Threaded ports to determine if they should initiate an
   * asynchronous update (still in development).
   */
  vtkMTimeType GetUpdateTime();

  /**
   * Return the actual size of the data in kibibytes (1024 bytes). This number
   * is valid only after the pipeline has updated. The memory size
   * returned is guaranteed to be greater than or equal to the
   * memory required to represent the data (e.g., extra space in
   * arrays, etc. are not included in the return value).
   */
  virtual unsigned long GetActualMemorySize();

  /**
   * Copy from the pipeline information to the data object's own information.
   * Called right before the main execution pass.
   */
  virtual void CopyInformationFromPipeline(vtkInformation* vtkNotUsed(info)) {}

  /**
   * Copy information from this data object to the pipeline information.
   * This is used by the vtkTrivialProducer that is created when someone
   * calls SetInputData() to connect a data object to a pipeline.
   */
  virtual void CopyInformationToPipeline(vtkInformation* vtkNotUsed(info)) {}

  /**
   * Return the information object within the input information object's
   * field data corresponding to the specified association
   * (FIELD_ASSOCIATION_POINTS or FIELD_ASSOCIATION_CELLS) and attribute
   * (SCALARS, VECTORS, NORMALS, TCOORDS, or TENSORS)
   */
  static vtkInformation* GetActiveFieldInformation(
    vtkInformation* info, int fieldAssociation, int attributeType);

  /**
   * Return the information object within the input information object's
   * field data corresponding to the specified association
   * (FIELD_ASSOCIATION_POINTS or FIELD_ASSOCIATION_CELLS) and name.
   */
  static vtkInformation* GetNamedFieldInformation(
    vtkInformation* info, int fieldAssociation, const char* name);

  /**
   * Remove the info associated with an array
   */
  static void RemoveNamedFieldInformation(
    vtkInformation* info, int fieldAssociation, const char* name);

  /**
   * Set the named array to be the active field for the specified type
   * (SCALARS, VECTORS, NORMALS, TCOORDS, or TENSORS) and association
   * (FIELD_ASSOCIATION_POINTS or FIELD_ASSOCIATION_CELLS).  Returns the
   * active field information object and creates on entry if one not found.
   */
  static vtkInformation* SetActiveAttribute(
    vtkInformation* info, int fieldAssociation, const char* attributeName, int attributeType);

  /**
   * Set the name, array type, number of components, and number of tuples
   * within the passed information object for the active attribute of type
   * attributeType (in specified association, FIELD_ASSOCIATION_POINTS or
   * FIELD_ASSOCIATION_CELLS).  If there is not an active attribute of the
   * specified type, an entry in the information object is created.  If
   * arrayType, numComponents, or numTuples equal to -1, or name=nullptr the
   * value is not changed.
   */
  static void SetActiveAttributeInfo(vtkInformation* info, int fieldAssociation, int attributeType,
    const char* name, int arrayType, int numComponents, int numTuples);

  /**
   * Convenience version of previous method for use (primarily) by the Imaging
   * filters. If arrayType or numComponents == -1, the value is not changed.
   */
  static void SetPointDataActiveScalarInfo(vtkInformation* info, int arrayType, int numComponents);

  /**
   * This method is called by the source when it executes to generate data.
   * It is sort of the opposite of ReleaseData.
   * It sets the DataReleased flag to 0, and sets a new UpdateTime.
   */
  void DataHasBeenGenerated();

  /**
   * make the output data ready for new data to be inserted. For most
   * objects we just call Initialize. But for vtkImageData we leave the old
   * data in case the memory can be reused.
   */
  virtual void PrepareForNewData() { this->Initialize(); }

  /**
   * The goal of the method is to copy the data up to the array pointers only.
   * The implementation is delegated to the differenent subclasses.
   * If you want to copy the actual data, @see DeepCopy.
   *
   * This method shallow copy the field data and copy the internal structure.
   */
  virtual void ShallowCopy(vtkDataObject* src);

  /**
   * The goal of the method is to copy the complete data from src into this object.
   * The implementation is delegated to the differenent subclasses.
   * If you want to copy the data up to the array pointers only, @see ShallowCopy.
   *
   * This method deep copy the field data and copy the internal structure.
   */
  virtual void DeepCopy(vtkDataObject* src);

  /**
   * The ExtentType will be left as VTK_PIECES_EXTENT for data objects
   * such as vtkPolyData and vtkUnstructuredGrid. The ExtentType will be
   * changed to VTK_3D_EXTENT for data objects with 3D structure such as
   * vtkImageData (and its subclass vtkStructuredPoints), vtkRectilinearGrid,
   * and vtkStructuredGrid. The default is the have an extent in pieces,
   * with only one piece (no streaming possible).
   */
  virtual int GetExtentType() VTK_FUTURE_CONST { return VTK_PIECES_EXTENT; }

  /**
   * This method crops the data object (if necessary) so that the extent
   * matches the update extent.
   */
  virtual void Crop(const int* updateExtent);

  /**
   * Possible values for the FIELD_ASSOCIATION information entry.
   */
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

  /**
   * Possible attribute types.
   * POINT_THEN_CELL is provided for consistency with FieldAssociations.
   */
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

  /**
   * Returns the attributes of the data object of the specified
   * attribute type. The type may be:
   * <ul>
   * <li>POINT  - Defined in vtkDataSet subclasses.
   * <li>CELL   - Defined in vtkDataSet subclasses.
   * <li>VERTEX - Defined in vtkGraph subclasses.
   * <li>EDGE   - Defined in vtkGraph subclasses.
   * <li>ROW    - Defined in vtkTable.
   * </ul>
   * The other attribute type, FIELD, will return nullptr since
   * field data is stored as a vtkFieldData instance, not a
   * vtkDataSetAttributes instance. To retrieve field data, use
   * GetAttributesAsFieldData.
   *
   * @warning This method NEEDS to be
   * overridden in subclasses to work as documented.
   * If not, it returns nullptr for any type but FIELD.
   */
  virtual vtkDataSetAttributes* GetAttributes(int type);

  /**
   * Returns the ghost arrays of the data object of the specified
   * attribute type. The type may be:
   * <ul>
   * <li>POINT    - Defined in vtkDataSet subclasses
   * <li>CELL   - Defined in vtkDataSet subclasses.
   * </ul>
   * The other attribute types, will return nullptr since
   * ghosts arrays are not defined for now outside of
   * point or cell.
   */
  virtual vtkUnsignedCharArray* GetGhostArray(int type);

  /**
   * Returns if this type of data object support ghost array for specified type.
   * The type may be:
   * <ul>
   * <li>POINT    - Defined in vtkDataSet subclasses
   * <li>CELL   - Defined in vtkDataSet subclasses.
   * </ul>
   * The other attribute types, will return false since
   * ghosts arrays are not defined for now outside of point or cell.
   * for vtkDataObject, this always return false but subclasses may override
   * this method and implement their own logic.
   */
  virtual bool SupportsGhostArray(int type);

  /**
   * Returns the attributes of the data object as a vtkFieldData.
   * This returns non-null values in all the same cases as GetAttributes,
   * in addition to the case of FIELD, which will return the field data
   * for any vtkDataObject subclass.
   */
  virtual vtkFieldData* GetAttributesAsFieldData(int type);

  /**
   * Retrieves the attribute type that an array came from.
   * This is useful for obtaining which attribute type a input array
   * to an algorithm came from (retrieved from GetInputAbstractArrayToProcesss).
   */
  virtual int GetAttributeTypeForArray(vtkAbstractArray* arr);

  /**
   * Get the number of elements for a specific attribute type (POINT, CELL, etc.).
   */
  virtual vtkIdType GetNumberOfElements(int type);

  /**
   * Possible values for the FIELD_OPERATION information entry.
   */
  enum FieldOperations
  {
    FIELD_OPERATION_PRESERVED,
    FIELD_OPERATION_REINTERPOLATED,
    FIELD_OPERATION_MODIFIED,
    FIELD_OPERATION_REMOVED
  };

  /**
   * Given an integer association type, this static method returns a string type
   * for the attribute (i.e. associationType = 0: returns "Points").
   */
  static const char* GetAssociationTypeAsString(int associationType);

  /**
   * Given a string association name, this static method returns an integer association type
   * for the attribute (i.e. associationName = "Points": returns 0).
   */
  static int GetAssociationTypeFromString(const char* associationName);

  /**
   * \ingroup InformationKeys
   */
  static vtkInformationStringKey* DATA_TYPE_NAME();
  /**
   * \ingroup InformationKeys
   */
  static vtkInformationDataObjectKey* DATA_OBJECT();
  /**
   * \ingroup InformationKeys
   */
  static vtkInformationIntegerKey* DATA_EXTENT_TYPE();
  /**
   * \ingroup InformationKeys
   */
  static vtkInformationIntegerPointerKey* DATA_EXTENT();
  /**
   * \ingroup InformationKeys
   */
  static vtkInformationIntegerVectorKey* ALL_PIECES_EXTENT();
  /**
   * \ingroup InformationKeys
   */
  static vtkInformationIntegerKey* DATA_PIECE_NUMBER();
  /**
   * \ingroup InformationKeys
   */
  static vtkInformationIntegerKey* DATA_NUMBER_OF_PIECES();
  /**
   * \ingroup InformationKeys
   */
  static vtkInformationIntegerKey* DATA_NUMBER_OF_GHOST_LEVELS();
  /**
   * \ingroup InformationKeys
   */
  static vtkInformationDoubleKey* DATA_TIME_STEP();
  /**
   * \ingroup InformationKeys
   */
  static vtkInformationInformationVectorKey* POINT_DATA_VECTOR();
  /**
   * \ingroup InformationKeys
   */
  static vtkInformationInformationVectorKey* CELL_DATA_VECTOR();
  /**
   * \ingroup InformationKeys
   */
  static vtkInformationInformationVectorKey* VERTEX_DATA_VECTOR();
  /**
   * \ingroup InformationKeys
   */
  static vtkInformationInformationVectorKey* EDGE_DATA_VECTOR();
  /**
   * \ingroup InformationKeys
   */
  static vtkInformationIntegerKey* FIELD_ARRAY_TYPE();
  /**
   * \ingroup InformationKeys
   */
  static vtkInformationIntegerKey* FIELD_ASSOCIATION();
  /**
   * \ingroup InformationKeys
   */
  static vtkInformationIntegerKey* FIELD_ATTRIBUTE_TYPE();
  /**
   * \ingroup InformationKeys
   */
  static vtkInformationIntegerKey* FIELD_ACTIVE_ATTRIBUTE();
  /**
   * \ingroup InformationKeys
   */
  static vtkInformationIntegerKey* FIELD_NUMBER_OF_COMPONENTS();
  /**
   * \ingroup InformationKeys
   */
  static vtkInformationIntegerKey* FIELD_NUMBER_OF_TUPLES();
  /**
   * \ingroup InformationKeys
   */
  static vtkInformationIntegerKey* FIELD_OPERATION();
  /**
   * \ingroup InformationKeys
   */
  static vtkInformationDoubleVectorKey* FIELD_RANGE();
  /**
   * \ingroup InformationKeys
   */
  static vtkInformationIntegerVectorKey* PIECE_EXTENT();
  /**
   * \ingroup InformationKeys
   */
  static vtkInformationStringKey* FIELD_NAME();
  /**
   * \ingroup InformationKeys
   */
  static vtkInformationDoubleVectorKey* ORIGIN();
  /**
   * \ingroup InformationKeys
   */
  static vtkInformationDoubleVectorKey* SPACING();
  /**
   * \ingroup InformationKeys
   */
  static vtkInformationDoubleVectorKey* DIRECTION();
  /**
   * \ingroup InformationKeys
   */
  static vtkInformationDoubleVectorKey* BOUNDING_BOX();

  // Key used to put SIL information in the output information by readers.
  /**
   * \ingroup InformationKeys
   */
  static vtkInformationDataObjectKey* SIL();

  ///@{
  /**
   * Retrieve an instance of this class from an information object.
   */
  static vtkDataObject* GetData(vtkInformation* info);
  static vtkDataObject* GetData(vtkInformationVector* v, int i = 0);
  ///@}

protected:
  vtkDataObject();
  ~vtkDataObject() override;

  // General field data associated with data object
  vtkFieldData* FieldData;

  // Keep track of data release during network execution
  vtkTypeBool DataReleased;

  // When was this data last generated?
  vtkTimeStamp UpdateTime;

  // Arbitrary extra information associated with this data object.
  vtkInformation* Information;

private:
  // Helper method for the ShallowCopy and DeepCopy methods.
  void InternalDataObjectCopy(vtkDataObject* src);

  vtkDataObject(const vtkDataObject&) = delete;
  void operator=(const vtkDataObject&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
