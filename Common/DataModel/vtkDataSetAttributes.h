// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkDataSetAttributes
 * @brief   represent and manipulate attribute data in a dataset
 *
 * vtkDataSetAttributes is a class that is used to represent and manipulate
 * attribute data (e.g., scalars, vectors, normals, texture coordinates,
 * tensors, global ids, pedigree ids, and field data).
 *
 * This adds to vtkFieldData the ability to pick one of the arrays from the
 * field as the currently active array for each attribute type. In other
 * words, you pick one array to be called "THE" Scalars, and then filters down
 * the pipeline will treat that array specially. For example vtkContourFilter
 * will contour "THE" Scalar array unless a different array is asked for.
 *
 * Additionally vtkDataSetAttributes provides methods that filters call to
 * pass data through, copy data into, and interpolate from Fields. PassData
 * passes entire arrays from the source to the destination. Copy passes
 * through some subset of the tuples from the source to the destination.
 * Interpolate interpolates from the chosen tuple(s) in the source data, using
 * the provided weights, to produce new tuples in the destination.
 * Each attribute type has pass, copy and interpolate "copy" flags that
 * can be set in the destination to choose which attribute arrays will be
 * transferred from the source to the destination.
 *
 * Finally this class provides a mechanism to determine which attributes a
 * group of sources have in common, and to copy tuples from a source into
 * the destination, for only those attributes that are held by all.
 *
 * @warning
 * vtkDataSetAttributes is not in general thread safe due to the use of its
 * vtkFieldData::BasicIterator RequiredArrays data member. The class
 * vtkArrayListTemplate augments vtkDataSetAttributes for thread safety.
 *
 * @sa vtkArrayListTemplate
 */

#ifndef vtkDataSetAttributes_h
#define vtkDataSetAttributes_h

#include "vtkCommonDataModelModule.h"      // For export macro
#include "vtkDataSetAttributesFieldList.h" // for vtkDataSetAttributesFieldList
#include "vtkFieldData.h"
#include "vtkWrappingHints.h" // For VTK_MARSHALMANUAL

VTK_ABI_NAMESPACE_BEGIN
class vtkLookupTable;

class VTKCOMMONDATAMODEL_EXPORT VTK_MARSHALMANUAL vtkDataSetAttributes : public vtkFieldData
{
public:
  /**
   * Construct object with copying turned on for all data.
   */
  static vtkDataSetAttributes* New();
  static vtkDataSetAttributes* ExtendedNew();

  vtkTypeMacro(vtkDataSetAttributes, vtkFieldData);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Initialize all of the object's data to nullptr
   * Also, clear the copy flags.
   */
  void Initialize() override;

  /**
   * Attributes have a chance to bring themselves up to date; right
   * now this is ignored.
   */
  virtual void Update() {}

  // -- shallow and deep copy -----------------------------------------------

  /**
   * Deep copy of data (i.e., create new data arrays and
   * copy from input data).
   * Ignores the copy flags but preserves them in the output.
   */
  void DeepCopy(vtkFieldData* pd) override;

  /**
   * Shallow copy of data (i.e., use reference counting).
   * Ignores the copy flags but preserves them in the output.
   */
  void ShallowCopy(vtkFieldData* pd) override;

  // -- attribute types -----------------------------------------------------

  // Always keep NUM_ATTRIBUTES as the last entry
  enum AttributeTypes
  {
    SCALARS = 0,
    VECTORS = 1,
    NORMALS = 2,
    TCOORDS = 3,
    TENSORS = 4,
    GLOBALIDS = 5,
    PEDIGREEIDS = 6,
    EDGEFLAG = 7,
    TANGENTS = 8,
    RATIONALWEIGHTS = 9,
    HIGHERORDERDEGREES = 10,
    PROCESSIDS = 11,
    NUM_ATTRIBUTES
  };

  enum AttributeLimitTypes
  {
    MAX,
    EXACT,
    NOLIMIT
  };

  // ----------- ghost points and ghost cells -------------------------------------------
  // The following bit fields are consistent with VisIt ghost zones specification
  // For details, see http://www.visitusers.org/index.php?title=Representing_ghost_data

  enum CellGhostTypes
  {
    DUPLICATECELL = 1,        // the cell is present on multiple processors
    HIGHCONNECTIVITYCELL = 2, // the cell has more neighbors than in a regular mesh
    LOWCONNECTIVITYCELL = 4,  // the cell has less neighbors than in a regular mesh
    REFINEDCELL = 8,          // other cells are present that refines it.
    EXTERIORCELL = 16,        // the cell is on the exterior of the data set
    HIDDENCELL =
      32 // the cell is needed to maintain connectivity, but the data values should be ignored.
  };

  enum PointGhostTypes
  {
    DUPLICATEPOINT = 1, // the cell is present on multiple processors
    HIDDENPOINT =
      2 // the point is needed to maintain connectivity, but the data values should be ignored.
  };

  // A vtkDataArray with this name must be of type vtkUnsignedCharArray.
  // Each value must be assigned according to the bit fields described in
  // PointGhostTypes or CellGhostType
  static const char* GhostArrayName() { return "vtkGhostType"; }

  //-----------------------------------------------------------------------------------

  ///@{
  /**
   * Set/Get the scalar data.
   */
  int SetScalars(vtkDataArray* da);
  int SetActiveScalars(const char* name);
  vtkDataArray* GetScalars();
  ///@}

  ///@{
  /**
   * Set/Get the vector data.
   */
  int SetVectors(vtkDataArray* da);
  int SetActiveVectors(const char* name);
  vtkDataArray* GetVectors();
  ///@}

  ///@{
  /**
   * Set/get the normal data.
   */
  int SetNormals(vtkDataArray* da);
  int SetActiveNormals(const char* name);
  vtkDataArray* GetNormals();
  ///@}

  ///@{
  /**
   * Set/get the tangent data.
   */
  int SetTangents(vtkDataArray* da);
  int SetActiveTangents(const char* name);
  vtkDataArray* GetTangents();
  ///@}

  ///@{
  /**
   * Set/Get the texture coordinate data.
   */
  int SetTCoords(vtkDataArray* da);
  int SetActiveTCoords(const char* name);
  vtkDataArray* GetTCoords();
  ///@}

  ///@{
  /**
   * Set/Get the tensor data.
   */
  int SetTensors(vtkDataArray* da);
  int SetActiveTensors(const char* name);
  vtkDataArray* GetTensors();
  ///@}

  ///@{
  /**
   * Set/Get the global id data.
   */
  int SetGlobalIds(vtkDataArray* da);
  int SetActiveGlobalIds(const char* name);
  vtkDataArray* GetGlobalIds();
  ///@}

  ///@{
  /**
   * Set/Get the pedigree id data.
   */
  int SetPedigreeIds(vtkAbstractArray* da);
  int SetActivePedigreeIds(const char* name);
  vtkAbstractArray* GetPedigreeIds();
  ///@}

  ///@{
  /**
   * Set/Get the rational weights data.
   */
  int SetRationalWeights(vtkDataArray* da);
  int SetActiveRationalWeights(const char* name);
  vtkDataArray* GetRationalWeights();
  ///@}

  ///@{
  /**
   * Set/Get the rational degrees data.
   */
  int SetHigherOrderDegrees(vtkDataArray* da);
  int SetActiveHigherOrderDegrees(const char* name);
  vtkDataArray* GetHigherOrderDegrees();
  ///@}

  ///@{
  /**
   * Set/Get the process id data.
   */
  int SetProcessIds(vtkDataArray* da);
  int SetActiveProcessIds(const char* name);
  vtkDataArray* GetProcessIds();
  ///@}

  ///@{
  /**
   * If the string is nullptr or empty, calls the alternate method
   * of the same name (that takes no parameters).
   * Otherwise, it will look for an array with the correct name.
   * If one exists, it is returned. Otherwise, nullptr is returned.
   */
  vtkDataArray* GetScalars(const char* name);
  vtkDataArray* GetVectors(const char* name);
  vtkDataArray* GetNormals(const char* name);
  vtkDataArray* GetTangents(const char* name);
  vtkDataArray* GetTCoords(const char* name);
  vtkDataArray* GetTensors(const char* name);
  vtkDataArray* GetGlobalIds(const char* name);
  vtkAbstractArray* GetPedigreeIds(const char* name);
  vtkDataArray* GetRationalWeights(const char* name);
  vtkDataArray* GetHigherOrderDegrees(const char* name);
  vtkDataArray* GetProcessIds(const char* name);
  ///@}

  /**
   * Make the array with the given name the active attribute.
   * Attribute types are:
   * vtkDataSetAttributes::SCALARS = 0
   * vtkDataSetAttributes::VECTORS = 1
   * vtkDataSetAttributes::NORMALS = 2
   * vtkDataSetAttributes::TCOORDS = 3
   * vtkDataSetAttributes::TENSORS = 4
   * vtkDataSetAttributes::GLOBALIDS = 5
   * vtkDataSetAttributes::PEDIGREEIDS = 6
   * vtkDataSetAttributes::EDGEFLAG = 7
   * vtkDataSetAttributes::TANGENTS = 8
   * vtkDataSetAttributes::RATIONALWEIGHTS = 9
   * vtkDataSetAttributes::HIGHERORDERDEGREES = 10
   * vtkDataSetAttributes::PROCESSIDS = 11
   * Returns the index of the array if successful, -1 if the array
   * is not in the list of arrays.
   */
  int SetActiveAttribute(const char* name, int attributeType);

  /**
   * Make the array with the given index the active attribute.
   * Returns the index of the array if successful, -1 if the array
   * is not in the list of arrays.
   */
  int SetActiveAttribute(int index, int attributeType);

  /**
   * Get the field data array indices corresponding to scalars,
   * vectors, tensors, etc.  The given buffer must be at least
   * NUM_ATTRIBUTES elements big.
   */
  void GetAttributeIndices(int* indexArray);

  /**
   * Determine whether a data array of index idx is considered a data set
   * attribute (i.e., scalar, vector, tensor, etc). Return less-than zero
   * if it is, otherwise an index 0<=idx<NUM_ATTRIBUTES to indicate
   * which attribute.
   */
  int IsArrayAnAttribute(int idx);

  /**
   * Set an array to use as the given attribute type (i.e.,
   * vtkDataSetAttributes::SCALAR, vtkDataSetAttributes::VECTOR,
   * vtkDataSetAttributes::TENSOR, etc.). If this attribute was
   * previously set to another array, that array is removed from the
   * vtkDataSetAttributes object and the array aa is used as the
   * attribute.

   * Returns the index of aa within the vtkDataSetAttributes object
   * (i.e., the index to pass to the method GetArray(int) to obtain
   * aa) if the attribute was set to aa successfully. If aa was
   * already set as the given attributeType, returns the index of
   * aa.

   * Returns -1 in the following cases:

   * - aa is nullptr (used to unset an attribute; not an error indicator)
   * - aa is not a subclass of vtkDataArray, unless the attributeType
   * is vtkDataSetAttributes::PEDIGREEIDS (error indicator)
   * - aa has a number of components incompatible with the attribute type
   * (error indicator)
   */
  int SetAttribute(vtkAbstractArray* aa, int attributeType);

  /**
   * Return an attribute given the attribute type
   * (see vtkDataSetAttributes::AttributeTypes).
   * Some attributes (such as PEDIGREEIDS) may not be vtkDataArray subclass,
   * so in that case use GetAbstractAttribute().
   */
  vtkDataArray* GetAttribute(int attributeType);

  /**
   * Return an attribute given the attribute type
   * (see vtkDataSetAttributes::AttributeTypes).
   * This is the same as GetAttribute(), except that the returned array
   * is a vtkAbstractArray instead of vtkDataArray.
   * Some attributes (such as PEDIGREEIDS) may not be vtkDataArray subclass.
   */
  vtkAbstractArray* GetAbstractAttribute(int attributeType);

  ///@{
  /**
   * Remove an array (with the given index) from the list of arrays.
   * Does nothing if the index is out of range.
   */
  using vtkFieldData::RemoveArray;
  void RemoveArray(int index) override;
  ///@}

  ///@{
  /**
   * Given an integer attribute type, this static method returns a string type
   * for the attribute (i.e. type = 0: returns "Scalars").
   */
  static const char* GetAttributeTypeAsString(int attributeType);
  static const char* GetLongAttributeTypeAsString(int attributeType);
  ///@}

  // -- attribute copy properties ------------------------------------------

  enum AttributeCopyOperations
  {
    COPYTUPLE = 0,
    INTERPOLATE = 1,
    PASSDATA = 2,
    ALLCOPY // all of the above
  };

  /**
   * Turn on/off the copying of attribute data.
   * ctype is one of the AttributeCopyOperations, and controls copy,
   * interpolate and passdata behavior.
   * For set, ctype=ALLCOPY means set all three flags to the same value.
   * For get, ctype=ALLCOPY returns true only if all three flags are true.

   * During copying, interpolation and passdata, the following rules are
   * followed for each array:
   * 1. If the copy/interpolate/pass flag for an attribute is set (on or off),
   * it is applied. This overrides rules 2 and 3.
   * 2. If the copy flag for an array is set (on or off), it is applied
   * This overrides rule 3.
   * 3. If CopyAllOn is set, copy the array.
   * If CopyAllOff is set, do not copy the array

   * For interpolation, the flag values can be as follows:
   * 0: Do not interpolate.
   * 1: Weighted interpolation.
   * 2: Nearest neighbor interpolation.
   */
  void SetCopyAttribute(int index, int value, int ctype = ALLCOPY);

  /**
   * Get the attribute copy flag for copy operation \c ctype of attribute
   * \c index.
   */
  int GetCopyAttribute(int index, int ctype);

  /// @copydoc vtkDataSetAttributes::SetCopyAttribute()
  void SetCopyScalars(vtkTypeBool i, int ctype = ALLCOPY);
  vtkTypeBool GetCopyScalars(int ctype = ALLCOPY);
  vtkBooleanMacro(CopyScalars, vtkTypeBool);

  /// @copydoc vtkDataSetAttributes::SetCopyAttribute()
  void SetCopyVectors(vtkTypeBool i, int ctype = ALLCOPY);
  vtkTypeBool GetCopyVectors(int ctype = ALLCOPY);
  vtkBooleanMacro(CopyVectors, vtkTypeBool);

  /// @copydoc vtkDataSetAttributes::SetCopyAttribute()
  void SetCopyNormals(vtkTypeBool i, int ctype = ALLCOPY);
  vtkTypeBool GetCopyNormals(int ctype = ALLCOPY);
  vtkBooleanMacro(CopyNormals, vtkTypeBool);

  /// @copydoc vtkDataSetAttributes::SetCopyAttribute()
  void SetCopyTangents(vtkTypeBool i, int ctype = ALLCOPY);
  vtkTypeBool GetCopyTangents(int ctype = ALLCOPY);
  vtkBooleanMacro(CopyTangents, vtkTypeBool);

  /// @copydoc vtkDataSetAttributes::SetCopyAttribute()
  void SetCopyTCoords(vtkTypeBool i, int ctype = ALLCOPY);
  vtkTypeBool GetCopyTCoords(int ctype = ALLCOPY);
  vtkBooleanMacro(CopyTCoords, vtkTypeBool);

  /// @copydoc vtkDataSetAttributes::SetCopyAttribute()
  void SetCopyTensors(vtkTypeBool i, int ctype = ALLCOPY);
  vtkTypeBool GetCopyTensors(int ctype = ALLCOPY);
  vtkBooleanMacro(CopyTensors, vtkTypeBool);

  /// @copydoc vtkDataSetAttributes::SetCopyAttribute()
  void SetCopyGlobalIds(vtkTypeBool i, int ctype = ALLCOPY);
  vtkTypeBool GetCopyGlobalIds(int ctype = ALLCOPY);
  vtkBooleanMacro(CopyGlobalIds, vtkTypeBool);

  /// @copydoc vtkDataSetAttributes::SetCopyAttribute()
  void SetCopyPedigreeIds(vtkTypeBool i, int ctype = ALLCOPY);
  vtkTypeBool GetCopyPedigreeIds(int ctype = ALLCOPY);
  vtkBooleanMacro(CopyPedigreeIds, vtkTypeBool);

  /// @copydoc vtkDataSetAttributes::SetCopyAttribute()
  void SetCopyRationalWeights(vtkTypeBool i, int ctype = ALLCOPY);
  vtkTypeBool GetCopyRationalWeights(int ctype = ALLCOPY);
  vtkBooleanMacro(CopyRationalWeights, vtkTypeBool);

  /// @copydoc vtkDataSetAttributes::SetCopyAttribute()
  void SetCopyHigherOrderDegrees(vtkTypeBool i, int ctype = ALLCOPY);
  vtkTypeBool GetCopyHigherOrderDegrees(int ctype = ALLCOPY);
  vtkBooleanMacro(CopyHigherOrderDegrees, vtkTypeBool);

  /// @copydoc vtkDataSetAttributes::SetCopyAttribute()
  void SetCopyProcessIds(vtkTypeBool i, int ctype = ALLCOPY);
  vtkTypeBool GetCopyProcessIds(int ctype = ALLCOPY);
  vtkBooleanMacro(CopyProcessIds, vtkTypeBool);

  /// @copydoc vtkDataSetAttributes::SetCopyAttribute()
  void CopyAllOn(int ctype = ALLCOPY) override;

  /// @copydoc vtkDataSetAttributes::SetCopyAttribute()
  void CopyAllOff(int ctype = ALLCOPY) override;

  // -- passthrough operations ----------------------------------------------

  /**
   * Pass entire arrays of input data through to output. Obey the "copy"
   * flags. When passing a field, the following copying rules are
   * followed: 1) Check if a field is an attribute, if yes and if there
   * is a PASSDATA copy flag for that attribute (on or off), obey the flag
   * for that attribute, ignore (2) and (3), 2) if there is a copy field for
   * that field (on or off), obey the flag, ignore (3) 3) obey
   * CopyAllOn/Off
   */
  void PassData(vtkFieldData* fd) override;

  // -- copytuple operations ------------------------------------------------

  ///@{
  /**
   * Allocates point data for point-by-point (or cell-by-cell) copy operation.
   * If sze=0, then use the input DataSetAttributes to create (i.e., find
   * initial size of) new objects; otherwise use the sze variable.
   * Note that pd HAS to be the vtkDataSetAttributes object which
   * will later be used with CopyData. If this is not the case,
   * consider using the alternative forms of CopyAllocate and CopyData.
   * ext is no longer used.
   * If shallowCopyArrays is true, input arrays are copied to the output
   * instead of new ones being allocated.
   */
  void CopyAllocate(vtkDataSetAttributes* pd, vtkIdType sze = 0, vtkIdType ext = 1000)
  {
    this->CopyAllocate(pd, sze, ext, 0);
  }
  void CopyAllocate(vtkDataSetAttributes* pd, vtkIdType sze, vtkIdType ext, int shallowCopyArrays);
  ///@}

  /**
   * Create a mapping between the input attributes and this object
   * so that methods like CopyData() and CopyStructuredData()
   * can be called. This method assumes that this object has the
   * same arrays as the input and that they are ordered the same
   * way (same array indices).
   */
  void SetupForCopy(vtkDataSetAttributes* pd);

  /**
   * This method is used to copy data arrays in images.
   * You should call CopyAllocate or SetupForCopy before
   * calling this method. If setSize is true, this method
   * will set the size of the output arrays according to
   * the output extent. This is required when CopyAllocate()
   * was used to setup output arrays.
   */
  void CopyStructuredData(
    vtkDataSetAttributes* inDsa, const int* inExt, const int* outExt, bool setSize = true);

  ///@{
  /**
   * Copy the attribute data from one id to another. Make sure CopyAllocate()
   * has been invoked before using this method. When copying a field,
   * the following copying rules are
   * followed: 1) Check if a field is an attribute, if yes and if there
   * is a COPYTUPLE copy flag for that attribute (on or off), obey the flag
   * for that attribute, ignore (2) and (3), 2) if there is a copy field for
   * that field (on or off), obey the flag, ignore (3) 3) obey
   * CopyAllOn/Off
   *
   * @warning This method is prone to compile-time ambiguity when called using `0` parameters.
   * To fix the ambiguity, please replace `0` by `vtkIdType(0)`.
   */
  void CopyData(vtkDataSetAttributes* fromPd, vtkIdType fromId, vtkIdType toId);
  void CopyData(vtkDataSetAttributes* fromPd, vtkIdList* fromIds, vtkIdList* toIds);
  void CopyData(vtkDataSetAttributes* fromPd, vtkIdList* fromIds, vtkIdType destStartId = 0);
  ///@}

  /**
   * Copy n consecutive attributes starting at srcStart from fromPd to this
   * container, starting at the dstStart location.
   * Note that memory allocation is performed as necessary to hold the data.
   */
  void CopyData(vtkDataSetAttributes* fromPd, vtkIdType dstStart, vtkIdType n, vtkIdType srcStart);

  ///@{
  /**
   * Copy a tuple (or set of tuples) of data from one data array to another.
   * This method assumes that the fromData and toData objects are of the
   * same type, and have the same number of components. This is true if you
   * invoke CopyAllocate() or InterpolateAllocate().
   */
  void CopyTuple(
    vtkAbstractArray* fromData, vtkAbstractArray* toData, vtkIdType fromId, vtkIdType toId);
  void CopyTuples(
    vtkAbstractArray* fromData, vtkAbstractArray* toData, vtkIdList* fromIds, vtkIdList* toIds);
  void CopyTuples(vtkAbstractArray* fromData, vtkAbstractArray* toData, vtkIdType dstStart,
    vtkIdType n, vtkIdType srcStart);
  ///@}

  // -- interpolate operations ----------------------------------------------

  ///@{
  /**
   * Initialize point interpolation method.
   * Note that pd HAS to be the vtkDataSetAttributes object which
   * will later be used with InterpolatePoint or InterpolateEdge.
   * ext is no longer used.
   * If shallowCopyArrays is true, input arrays are copied to the output
   * instead of new ones being allocated.
   */
  void InterpolateAllocate(vtkDataSetAttributes* pd, vtkIdType sze = 0, vtkIdType ext = 1000)
  {
    this->InterpolateAllocate(pd, sze, ext, 0);
  }
  void InterpolateAllocate(
    vtkDataSetAttributes* pd, vtkIdType sze, vtkIdType ext, int shallowCopyArrays);
  ///@}

  /**
   * Interpolate data set attributes from other data set attributes
   * given cell or point ids and associated interpolation weights.
   * If the INTERPOLATION copy flag is set to 0 for an array, interpolation
   * is prevented. If the flag is set to 1, weighted interpolation occurs.
   * If the flag is set to 2, nearest neighbor interpolation is used.
   */
  void InterpolatePoint(
    vtkDataSetAttributes* fromPd, vtkIdType toId, vtkIdList* ids, double* weights);

  /**
   * Interpolate data from the two points p1,p2 (forming an edge) and an
   * interpolation factor, t, along the edge. The weight ranges from (0,1),
   * with t=0 located at p1. Make sure that the method InterpolateAllocate()
   * has been invoked before using this method.
   * If the INTERPOLATION copy flag is set to 0 for an array, interpolation
   * is prevented. If the flag is set to 1, weighted interpolation occurs.
   * If the flag is set to 2, nearest neighbor interpolation is used.
   */
  void InterpolateEdge(
    vtkDataSetAttributes* fromPd, vtkIdType toId, vtkIdType p1, vtkIdType p2, double t);

  /**
   * Interpolate data from the same id (point or cell) at different points
   * in time (parameter t). Two input data set attributes objects are input.
   * The parameter t lies between (0<=t<=1). IMPORTANT: it is assumed that
   * the number of attributes and number of components is the same for both
   * from1 and from2, and the type of data for from1 and from2 are the same.
   * Make sure that the method InterpolateAllocate() has been invoked before
   * using this method.
   * If the INTERPOLATION copy flag is set to 0 for an array, interpolation
   * is prevented. If the flag is set to 1, weighted interpolation occurs.
   * If the flag is set to 2, nearest neighbor interpolation is used.
   */
  void InterpolateTime(
    vtkDataSetAttributes* from1, vtkDataSetAttributes* from2, vtkIdType id, double t);

  using FieldList = vtkDataSetAttributesFieldList;

  // field list copy operations ------------------------------------------

  /**
   * A special form of CopyAllocate() to be used with FieldLists. Use it
   * when you are copying data from a set of vtkDataSetAttributes.
   */
  void CopyAllocate(vtkDataSetAttributes::FieldList& list, vtkIdType sze = 0, vtkIdType ext = 1000);

  /**
   * Special forms of CopyData() to be used with FieldLists. Use it when
   * you are copying data from a set of vtkDataSetAttributes. Make sure
   * that you have called the special form of CopyAllocate that accepts
   * FieldLists.
   */
  void CopyData(vtkDataSetAttributes::FieldList& list, vtkDataSetAttributes* dsa, int idx,
    vtkIdType fromId, vtkIdType toId);
  void CopyData(vtkDataSetAttributes::FieldList& list, vtkDataSetAttributes* dsa, int idx,
    vtkIdType dstStart, vtkIdType n, vtkIdType srcStart);

  /**
   * A special form of InterpolateAllocate() to be used with FieldLists. Use it
   * when you are interpolating data from a set of vtkDataSetAttributes.
   * \c Warning: This does not copy the Information object associated with
   * each data array. This behavior may change in the future.
   */
  void InterpolateAllocate(
    vtkDataSetAttributes::FieldList& list, vtkIdType sze = 0, vtkIdType ext = 1000);

  /**
   * Interpolate data set attributes from other data set attributes
   * given cell or point ids and associated interpolation weights.
   * Make sure that special form of InterpolateAllocate() that accepts
   * FieldList has been used.
   */
  void InterpolatePoint(vtkDataSetAttributes::FieldList& list, vtkDataSetAttributes* fromPd,
    int idx, vtkIdType toId, vtkIdList* ids, double* weights);

protected:
  vtkDataSetAttributes();
  ~vtkDataSetAttributes() override;

  void InternalCopyAllocate(vtkDataSetAttributes* pd, int ctype, vtkIdType sze = 0,
    vtkIdType ext = 1000, int shallowCopyArrays = 0, bool createNewArrays = true);

  /**
   * Initialize all of the object's data to nullptr
   */
  void InitializeFields() override;

  int AttributeIndices[NUM_ATTRIBUTES];            // index to attribute array in field data
  int CopyAttributeFlags[ALLCOPY][NUM_ATTRIBUTES]; // copy flag for attribute data

  friend struct ArrayList; // Friend to base class in vtkArrayListTemplate
  vtkFieldData::BasicIterator RequiredArrays;
  int* TargetIndices;

  static const int NumberOfAttributeComponents[NUM_ATTRIBUTES];
  static const int AttributeLimits[NUM_ATTRIBUTES];
  static const char AttributeNames[NUM_ATTRIBUTES][19];
  static const char LongAttributeNames[NUM_ATTRIBUTES][42];

private:
  static int CheckNumberOfComponents(vtkAbstractArray* da, int attributeType);

  vtkFieldData::BasicIterator ComputeRequiredArrays(vtkDataSetAttributes* pd, int ctype);

  vtkDataSetAttributes(const vtkDataSetAttributes&) = delete;
  void operator=(const vtkDataSetAttributes&) = delete;

  friend class vtkDataSetAttributesFieldList;
};

VTK_ABI_NAMESPACE_END
#endif
