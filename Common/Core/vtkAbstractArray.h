/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAbstractArray.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
//
// .NAME vtkAbstractArray - Abstract superclass for all arrays
//
// .SECTION Description
//
// vtkAbstractArray is an abstract superclass for data array objects.
// This class defines an API that all subclasses must support.  The
// data type must be assignable and copy-constructible, but no other
// assumptions about its type are made.  Most of the subclasses of
// this array deal with numeric data either as scalars or tuples of
// scalars.  A program can use the IsNumeric() method to check whether
// an instance of vtkAbstractArray contains numbers.  It is also
// possible to test for this by attempting to SafeDownCast an array to
// an instance of vtkDataArray, although this assumes that all numeric
// arrays will always be descended from vtkDataArray.
//
// <p>
//
// Every array has a character-string name. The naming of the array
// occurs automatically when it is instantiated, but you are free to
// change this name using the SetName() method.  (The array name is
// used for data manipulation.)
//
// .SECTION See Also
// vtkDataArray vtkStringArray vtkCellArray

#ifndef __vtkAbstractArray_h
#define __vtkAbstractArray_h

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkObject.h"
#include "vtkVariant.h" // for variant arguments

class vtkArrayIterator;
class vtkDataArray;
class vtkIdList;
class vtkIdTypeArray;
class vtkInformation;
class vtkInformationDoubleVectorKey;
class vtkInformationIntegerKey;
class vtkInformationInformationVectorKey;
class vtkInformationVariantVectorKey;
class vtkVariantArray;

class VTKCOMMONCORE_EXPORT vtkAbstractArray : public vtkObject
{
public:
  vtkTypeMacro(vtkAbstractArray,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Allocate memory for this array. Delete old storage only if necessary.
  // Note that ext is no longer used.
  virtual int Allocate(vtkIdType sz, vtkIdType ext=1000) = 0;

  // Description:
  // Release storage and reset array to initial state.
  virtual void Initialize() = 0;

  // Description:
  // Return the underlying data type. An integer indicating data type is
  // returned as specified in vtkSetGet.h.
  virtual int GetDataType() =0;

  // Description:
  // Return the size of the underlying data type.  For a bit, 0 is
  // returned.  For string 0 is returned. Arrays with variable length
  // components return 0.
  virtual int GetDataTypeSize() = 0;
  static int GetDataTypeSize(int type);

  // Description:
  // Return the size, in bytes, of the lowest-level element of an
  // array.  For vtkDataArray and subclasses this is the size of the
  // data type.  For vtkStringArray, this is
  // sizeof(vtkStdString::value_type), which winds up being
  // sizeof(char).
  virtual int GetElementComponentSize() = 0;

  // Description:
  // Set/Get the dimension (n) of the components. Must be >= 1. Make sure that
  // this is set before allocation.
  vtkSetClampMacro(NumberOfComponents, int, 1, VTK_INT_MAX);
  int GetNumberOfComponents() { return this->NumberOfComponents; }

  // Description:
  // Set the name for a component. Must be >= 1.
  void SetComponentName( vtkIdType component, const char *name );

  //Description:
  // Get the component name for a given component.
  // Note: will return the actual string that is stored
  const char* GetComponentName( vtkIdType component );

  // Description:
  // Returns if any component has had a name assigned
  bool HasAComponentName();

  // Description:
  // Copies the component names from the inputed array to the current array
  // make sure that the current array has the same number of components as the input array
  int CopyComponentNames( vtkAbstractArray *da );

  // Description:
  // Set the number of tuples (a component group) in the array. Note that
  // this may allocate space depending on the number of components.
  // Also note that if allocation is performed no copy is performed so
  // existing data will be lost (if data conservation is sought, one may
  // use the Resize method instead).
  virtual void SetNumberOfTuples(vtkIdType number) = 0;

  // Description:
  // Get the number of tuples (a component group) in the array.
  vtkIdType GetNumberOfTuples()
    {return (this->MaxId + 1)/this->NumberOfComponents;}

  // Description:
  // Set the tuple at the ith location using the jth tuple in the source array.
  // This method assumes that the two arrays have the same type
  // and structure. Note that range checking and memory allocation is not
  // performed; use in conjunction with SetNumberOfTuples() to allocate space.
  virtual void SetTuple(vtkIdType i, vtkIdType j, vtkAbstractArray* source) = 0;

  // Description:
  // Insert the jth tuple in the source array, at ith location in this array.
  // Note that memory allocation is performed as necessary to hold the data.
  virtual void InsertTuple(vtkIdType i, vtkIdType j, vtkAbstractArray* source) = 0;

  // Description:
  // Insert the jth tuple in the source array, at the end in this array.
  // Note that memory allocation is performed as necessary to hold the data.
  // Returns the location at which the data was inserted.
  virtual vtkIdType InsertNextTuple(vtkIdType j, vtkAbstractArray* source) = 0;

  // Description:
  // Given a list of point ids, return an array of tuples.
  // You must insure that the output array has been previously
  // allocated with enough space to hold the data.
  virtual void GetTuples(vtkIdList *ptIds, vtkAbstractArray* output);

  // Description:
  // Get the tuples for the range of points ids specified
  // (i.e., p1->p2 inclusive). You must insure that the output array has
  // been previously allocated with enough space to hold the data.
  virtual void GetTuples(vtkIdType p1, vtkIdType p2, vtkAbstractArray *output);

  // Description:
  // Return a void pointer. For image pipeline interface and other
  // special pointer manipulation.
  virtual void *GetVoidPointer(vtkIdType id) = 0;

  // Description:
  // Deep copy of data. Implementation left to subclasses, which
  // should support as many type conversions as possible given the
  // data type.
  //
  // Subclasses should call vtkAbstractArray::DeepCopy() so that the
  // information object (if one exists) is copied from \a da.
  virtual void DeepCopy(vtkAbstractArray* da);

  // Description:
  // Set the ith tuple in this array as the interpolated tuple value,
  // given the ptIndices in the source array and associated
  // interpolation weights.
  // This method assumes that the two arrays are of the same type
  // and strcuture.
  virtual void InterpolateTuple(vtkIdType i, vtkIdList *ptIndices,
    vtkAbstractArray* source,  double* weights) = 0;

  // Description
  // Insert the ith tuple in this array as interpolated from the two values,
  // p1 and p2, and an interpolation factor, t.
  // The interpolation factor ranges from (0,1),
  // with t=0 located at p1. This method assumes that the three arrays are of
  // the same type. p1 is value at index id1 in source1, while, p2 is
  // value at index id2 in source2.
  virtual void InterpolateTuple(vtkIdType i,
    vtkIdType id1, vtkAbstractArray* source1,
    vtkIdType id2, vtkAbstractArray* source2, double t) =0;

  // Description:
  // Free any unnecessary memory.
  // Description:
  // Resize object to just fit data requirement. Reclaims extra memory.
  virtual void Squeeze() = 0;

  // Description:
  // Resize the array while conserving the data.  Returns 1 if
  // resizing succeeded and 0 otherwise.
  virtual int Resize(vtkIdType numTuples) = 0;

  // Description:
  // Reset to an empty state, without freeing any memory.
  void Reset()
    {this->MaxId = -1;}

  // Description:
  // Return the size of the data.
  vtkIdType GetSize()
  {return this->Size;}

  // Description:
  // What is the maximum id currently in the array.
  vtkIdType GetMaxId()
    {return this->MaxId;}

  // Description:
  // This method lets the user specify data to be held by the array.  The
  // array argument is a pointer to the data.  size is the size of
  // the array supplied by the user.  Set save to 1 to keep the class
  // from deleting the array when it cleans up or reallocates memory.
  // The class uses the actual array provided; it does not copy the data
  // from the supplied array.
  virtual void SetVoidArray(void *vtkNotUsed(array),
                            vtkIdType vtkNotUsed(size),
                            int vtkNotUsed(save)) =0;

  // Description:
  // This method copies the array data to the void pointer specified
  // by the user.  It is up to the user to allocate enough memory for
  // the void pointer.
  virtual void ExportToVoidPointer(void *vtkNotUsed(out_ptr)) {}

  // Description:
  // Return the memory in kilobytes consumed by this data array. Used to
  // support streaming and reading/writing data. The value returned is
  // guaranteed to be greater than or equal to the memory required to
  // actually represent the data represented by this object. The
  // information returned is valid only after the pipeline has
  // been updated.
  virtual unsigned long GetActualMemorySize() = 0;

  // Description:
  // Set/get array's name
  vtkSetStringMacro(Name);
  vtkGetStringMacro(Name);

  // Description:
  // Get the name of a data type as a string.
  virtual const char *GetDataTypeAsString( void )
    { return vtkImageScalarTypeNameMacro( this->GetDataType() ); }

  // Description:
  // Creates an array for dataType where dataType is one of
  // VTK_BIT, VTK_CHAR, VTK_UNSIGNED_CHAR, VTK_SHORT,
  // VTK_UNSIGNED_SHORT, VTK_INT, VTK_UNSIGNED_INT, VTK_LONG,
  // VTK_UNSIGNED_LONG, VTK_DOUBLE, VTK_DOUBLE, VTK_ID_TYPE,
  // VTK_STRING.
  // Note that the data array returned has to be deleted by the
  // user.
  static vtkAbstractArray* CreateArray(int dataType);

  // Description:
  // This method is here to make backward compatibility easier.  It
  // must return true if and only if an array contains numeric data.
  virtual int IsNumeric() = 0;

  // Description:
  // Subclasses must override this method and provide the right kind
  // of templated vtkArrayIteratorTemplate.
  virtual vtkArrayIterator* NewIterator() = 0;

  // Description:
  // Returns the size of the data in DataTypeSize units. Thus, the
  // number of bytes for the data can be computed by GetDataSize() *
  // GetDataTypeSize(). Non-contiguous or variable- size arrays need
  // to override this method.
  virtual vtkIdType GetDataSize()
    {
    return this->GetNumberOfComponents() * this->GetNumberOfTuples();
    }

  // Description:
  // Return the indices where a specific value appears.
  virtual vtkIdType LookupValue(vtkVariant value) = 0;
  virtual void LookupValue(vtkVariant value, vtkIdList* ids) = 0;

  // Description:
  // Retrieve value from the array as a variant.
  virtual vtkVariant GetVariantValue(vtkIdType idx);

  // Description:
  // Insert a value into the array from a variant.  This method does
  // bounds checking.
  virtual void InsertVariantValue(vtkIdType idx, vtkVariant value);

  // Description:
  // Set a value in the array from a variant.  This method does NOT do
  // bounds checking.
  virtual void SetVariantValue(vtkIdType idx, vtkVariant value) = 0;

  // Description:
  // Tell the array explicitly that the data has changed.
  // This is only necessary to call when you modify the array contents
  // without using the array's API (i.e. you retrieve a pointer to the
  // data and modify the array contents).  You need to call this so that
  // the fast lookup will know to rebuild itself.  Otherwise, the lookup
  // functions will give incorrect results.
  virtual void DataChanged() = 0;

  // Description:
  // Delete the associated fast lookup data structure on this array,
  // if it exists.  The lookup will be rebuilt on the next call to a lookup
  // function.
  virtual void ClearLookup() = 0;

  // Description:
  // Populate the given vtkVariantArray with a set of distinct values taken on
  // by the requested component (or, when passed -1, by the tuples as a whole).
  // If the set of prominent values has more than 32 entries, then the array
  // is assumed to be continuous in nature and no values are returned.
  //
  // This method takes 2 parameters: \a uncertainty and \a minimumProminence.
  // Note that this set of returned values may not be complete if
  // \a uncertainty and \a minimumProminence are both larger than 0.0;
  // in order to perform interactively, a subsample of the array is
  // used to determine the set of values.
  //
  // The first parameter (\a uncertainty, U) is the maximum acceptable
  // probability that a prominent value will not be detected.
  // Setting this to 0 will cause every value in the array to be examined.
  //
  // The second parameter (\a minimumProminence, P) specifies the smallest
  // relative frequency (in [0,1]) with which a value in the array may
  // occur and still be considered prominent. Setting this to 0
  // will force every value in the array to be traversed.
  // Using numbers close to 0 for this parameter quickly causes
  // the number of samples required to obtain the given uncertainty to
  // subsume the entire array, as rare occurrences require frequent
  // sampling to detect.
  //
  // For an array with T tuples and given uncertainty U and mininumum
  // prominence P, we sample N values, with N = f(T; P, U).
  // We want f to be sublinear in T in order to interactively handle large
  // arrays; in practice, we can make f independent of T:
  // \f$ N >= \frac{5}{P}\mathrm{ln}\left(\frac{1}{PU}) \f$,
  // but note that small values of P are costly to achieve.
  // The default parameters will locate prominent values that occur at least
  // 1 out of every 1000 samples with a confidence of 0.999999 (= 1 - 1e6).
  // Thanks to Seshadri Comandur (Sandia National Laboratories) for the
  // bounds on the number of samples.
  //
  // The first time this is called, the array is examined and unique values
  // are stored in the vtkInformation object associated with the array.
  // The list of unique values will be updated on subsequent calls only if
  // the array's MTime is newer than the associated vtkInformation object or
  // if better sampling (lower \a uncertainty or \a minimumProminence) is
  // requested.
  // The DISCRETE_VALUE_SAMPLE_PARAMETERS() information key is used to
  // store the numbers which produced any current set of prominent values.
  //
  // Also, note that every value encountered is reported and counts toward
  // the maximum of 32 distinct values, regardless of the value's frequency.
  // This is required for an efficient implementation.
  // Use the vtkOrderStatistics filter if you wish to threshold the set of
  // distinct values to eliminate "unprominent" (infrequently-occurring)
  // values.
  virtual void GetProminentComponentValues(int comp, vtkVariantArray* values,
    double uncertainty = 1.e-6, double minimumProminence = 1.e-3);

  // TODO: Implement these lookup functions also.
  //virtual void LookupRange(vtkVariant min, vtkVariant max, vtkIdList* ids,
  //  bool includeMin = true, bool includeMax = true) = 0;
  //virtual void LookupGreaterThan(vtkVariant min, vtkIdList* ids, bool includeMin = false) = 0;
  //virtual void LookupLessThan(vtkVariant max, vtkIdList* ids, bool includeMax = false) = 0;

  // Description:
  // Get an information object that can be used to annotate the array.
  // This will always return an instance of vtkInformation, if one is
  // not currently associated with the array it will be created.
  vtkInformation* GetInformation();
  // Description:
  // Inquire if this array has an instance of vtkInformation
  // already associated with it.
  bool HasInformation(){ return this->Information!=0; }
  //BTX
  // Description:
  // Copy information instance. Arrays use information objects
  // in a variety of ways. It is important to have flexibility in
  // this regard because certain keys should not be coppied, while
  // others must be.
  //
  // NOTE: Subclasses must always call their superclass's CopyInformation
  // method, so that all classes in the hierarchy get a chance to remove
  // keys they do not wish to be coppied. The subclass will not need to
  // explicilty copy the keys as it's handled here.
  virtual int CopyInformation(vtkInformation *infoFrom, int deep=1);
  //ETX

  // Description:
  // This key is a hint to end user interface that this array
  // is internal and should not be shown to the end user.
  static vtkInformationIntegerKey* GUI_HIDE();

  // Description:
  // This key is used to hold a vector of COMPONENT_VALUES (and, for
  // vtkDataArray subclasses, COMPONENT_RANGE) keys -- one
  // for each component of the array.  You may add additional per-component
  // key-value pairs to information objects in this vector. However if you
  // do so, you must be sure to either (1) set COMPONENT_VALUES to
  // an invalid variant and set COMPONENT_RANGE to
  // {VTK_DOUBLE_MAX, VTK_DOUBLE_MIN} or (2) call ComputeUniqueValues(component)
  // and ComputeRange(component) <b>before</b> modifying the information object.
  // Otherwise it is possible for modifications to the array to take place
  // without the bounds on the component being updated since the modification
  // time of the vtkInformation object is used to determine when the
  // COMPONENT_RANGE values are out of date.
  static vtkInformationInformationVectorKey* PER_COMPONENT();

  // Description:
  // A key used to hold discrete values taken on either by the tuples of the
  // array (when present in this->GetInformation()) or individual components
  // (when present in one entry of the PER_COMPONENT() information vector).
  static vtkInformationVariantVectorKey* DISCRETE_VALUES();

  // Description:
  // A key used to hold conditions under which cached discrete values were generated;
  // the value is a 2-vector of doubles.
  // The first entry corresponds to the maximum uncertainty that prominent values
  // exist but have not been detected. The second entry corresponds to the smallest
  // relative frequency a value is allowed to have and still appear on the list.
  static vtkInformationDoubleVectorKey* DISCRETE_VALUE_SAMPLE_PARAMETERS();

  enum {
    MAX_DISCRETE_VALUES = 32
  };

protected:
  // Construct object with default tuple dimension (number of components) of 1.
  vtkAbstractArray(vtkIdType numComp=1);
  ~vtkAbstractArray();

  // Description:
  // Set an information object that can be used to annotate the array.
  // Use this with caution as array instances depend on persistence of
  // information keys. See CopyInformation.
  virtual void SetInformation( vtkInformation* );

  // Description:
  // Obtain the set of unique values taken on by each component of the array,
  // as well as by the tuples of the array.
  //
  // The results are stored in the PER_COMPONENT() vtkInformation objects
  // using the DISCRETE_VALUES() key.
  // If the key is present but stores 0 values, the array either has no
  // entries or does not behave as a discrete set.
  // If the key is not present, the array has not been examined for
  // distinct values or has been modified since the last examination.
  virtual void UpdateDiscreteValueSet(double uncertainty, double minProminence);

  vtkIdType Size;         // allocated size of data
  vtkIdType MaxId;        // maximum index inserted thus far
  int NumberOfComponents; // the number of components per tuple

  char* Name;

  bool RebuildArray;      // whether to rebuild the fast lookup data structure.

  vtkInformation* Information;

  //BTX
  class vtkInternalComponentNames;
  vtkInternalComponentNames* ComponentNames; //names for each component
  //ETX

private:
  vtkAbstractArray(const vtkAbstractArray&);  // Not implemented.
  void operator=(const vtkAbstractArray&);  // Not implemented.
};

#endif
