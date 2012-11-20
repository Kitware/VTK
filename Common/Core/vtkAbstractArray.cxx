/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAbstractArray.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkAbstractArray.h"

#include "vtkBitArray.h"
#include "vtkCharArray.h"
#include "vtkDoubleArray.h"
#include "vtkFloatArray.h"
#include "vtkIdList.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkIntArray.h"
#include "vtkLongArray.h"
#include "vtkMath.h"
#include "vtkMinimalStandardRandomSequence.h"
#include "vtkShortArray.h"
#include "vtkSignedCharArray.h"
#include "vtkStringArray.h"
#include "vtkUnicodeStringArray.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnsignedIntArray.h"
#include "vtkUnsignedLongArray.h"
#include "vtkUnsignedShortArray.h"
#include "vtkVariantArray.h"
#include "vtkInformationVector.h"
#include "vtkInformationDoubleVectorKey.h"
#include "vtkInformationIntegerKey.h"
#include "vtkInformationInformationVectorKey.h"
#include "vtkInformationVariantVectorKey.h"
#include "vtkNew.h"
#include "vtkUnicodeString.h" // for vtkSuperExtraExtendedTemplateMacro

#if defined(VTK_TYPE_USE_LONG_LONG)
# include "vtkLongLongArray.h"
# include "vtkUnsignedLongLongArray.h"
#endif

#if defined(VTK_TYPE_USE___INT64)
# include "vtk__Int64Array.h"
# if defined(VTK_TYPE_CONVERT_UI64_TO_DOUBLE)
#  include "vtkUnsigned__Int64Array.h"
# endif
#endif

#include <set>
#include <cmath>

vtkInformationKeyMacro(vtkAbstractArray, GUI_HIDE, Integer);
vtkInformationKeyMacro(vtkAbstractArray, PER_COMPONENT, InformationVector);
vtkInformationKeyMacro(vtkAbstractArray, DISCRETE_VALUES, VariantVector);
vtkInformationKeyRestrictedMacro(vtkAbstractArray, DISCRETE_VALUE_SAMPLE_PARAMETERS, DoubleVector, 2);

namespace
{
  typedef  std::vector< vtkStdString* > vtkInternalComponentNameBase;
}
class vtkAbstractArray::vtkInternalComponentNames : public vtkInternalComponentNameBase {};

//----------------------------------------------------------------------------
// Construct object with sane defaults.
vtkAbstractArray::vtkAbstractArray(vtkIdType vtkNotUsed(numComp))
{
  this->Size = 0;
  this->MaxId = -1;
  this->NumberOfComponents = 1;
  this->Name = NULL;
  this->Information = NULL;

  this->ComponentNames = NULL;
}

//----------------------------------------------------------------------------
vtkAbstractArray::~vtkAbstractArray()
{
  if ( this->ComponentNames )
    {
    for ( unsigned int i=0; i < this->ComponentNames->size(); ++i)
      {
      if ( this->ComponentNames->at(i) )
        {
        delete this->ComponentNames->at(i);
        }
      }
    this->ComponentNames->clear();
    delete this->ComponentNames;
    this->ComponentNames = NULL;
    }

  this->SetName(NULL);
  this->SetInformation(NULL);
}

//----------------------------------------------------------------------------
void vtkAbstractArray::SetComponentName( vtkIdType component, const char *name )
  {
  if ( component < 0 || name == NULL )
    {
    return;
    }
  unsigned int index = static_cast<unsigned int>( component );
  if ( this->ComponentNames == NULL )
    {
    //delayed allocate
    this->ComponentNames = new vtkAbstractArray::vtkInternalComponentNames();
    }

  if ( index == this->ComponentNames->size() )
    {
    //the array isn't large enough, so we will resize
    this->ComponentNames->push_back( new vtkStdString(name) );
    return;
    }
  else if ( index > this->ComponentNames->size() )
    {
    this->ComponentNames->resize( index+1, NULL );
    }

  //replace an exisiting element
  vtkStdString *compName = this->ComponentNames->at(index);
  if ( !compName )
    {
    compName = new vtkStdString(name);
    this->ComponentNames->at(index) = compName;
    }
  else
    {
    compName->assign( name );
    }
  }

//----------------------------------------------------------------------------
const char* vtkAbstractArray::GetComponentName( vtkIdType component )
{
  unsigned int index = static_cast<unsigned int>( component );
  if ( !this->ComponentNames || component < 0 ||
    index >= this->ComponentNames->size() )
    {
    //make sure we have valid vector
    return NULL;
    }

  vtkStdString *compName = this->ComponentNames->at( index );
  return ( compName ) ? compName->c_str() : NULL;
  }

//----------------------------------------------------------------------------
bool vtkAbstractArray::HasAComponentName()
{
  return (this->ComponentNames) ? ( this->ComponentNames->size() > 0 ) : 0;
}

//----------------------------------------------------------------------------
int vtkAbstractArray::CopyComponentNames( vtkAbstractArray *da )
{
  if (  da && da != this && da->ComponentNames )
    {
    //clear the vector of the all data
    if ( !this->ComponentNames )
      {
      this->ComponentNames = new vtkAbstractArray::vtkInternalComponentNames();
      }

    //copy the passed in components
    for ( unsigned int i=0; i < this->ComponentNames->size(); ++i)
      {
      if ( this->ComponentNames->at(i) )
        {
        delete this->ComponentNames->at(i);
        }
      }
    this->ComponentNames->clear();
    this->ComponentNames->reserve( da->ComponentNames->size() );
    const char *name;
    for ( unsigned int i = 0; i < da->ComponentNames->size(); ++i )
      {
      name = da->GetComponentName(i);
      if ( name )
        {
        this->SetComponentName(i, name);
        }
      }
    return 1;
    }
  return 0;
}


//----------------------------------------------------------------------------
void vtkAbstractArray::SetInformation(vtkInformation *args)
{
  // Same as in vtkCxxSetObjectMacro, but no Modified() so that
  // this doesn't cause extra pipeline updates.
  vtkDebugMacro(<< this->GetClassName() << " (" << this
      << "): setting Information to " << args );
  if (this->Information != args)
    {
    vtkInformation* tempSGMacroVar = this->Information;
    this->Information = args;
    if (this->Information != NULL) { this->Information->Register(this); }
    if (tempSGMacroVar != NULL)
      {
      tempSGMacroVar->UnRegister(this);
      }
    }
}

//----------------------------------------------------------------------------
void vtkAbstractArray::GetTuples(vtkIdList* ptIds, vtkAbstractArray* aa)
{
  if (aa->GetNumberOfComponents() != this->GetNumberOfComponents())
    {
    vtkWarningMacro("Number of components for input and output do not match.");
    return;
    }
  // Here we give the slowest implementation. Subclasses can override
  // to use the knowledge about the data.
  vtkIdType num = ptIds->GetNumberOfIds();
  for (vtkIdType i = 0; i < num; i++)
    {
    aa->SetTuple(i, ptIds->GetId(i), this);
    }
}

//----------------------------------------------------------------------------
void vtkAbstractArray::GetTuples(vtkIdType p1, vtkIdType p2,
  vtkAbstractArray* aa)
{
  if (aa->GetNumberOfComponents() != this->GetNumberOfComponents())
    {
    vtkWarningMacro("Number of components for input and output do not match.");
    return;
    }

  // Here we give the slowest implementation. Subclasses can override
  // to use the knowledge about the data.
  vtkIdType num = p2 - p1 + 1;
  for (vtkIdType i = 0; i < num; i++)
    {
    aa->SetTuple(i, (p1+i), this);
    }
}

//----------------------------------------------------------------------------
void vtkAbstractArray::DeepCopy( vtkAbstractArray* da )
{
  if (da && da->HasInformation() && da!=this)
    {
    this->CopyInformation(da->GetInformation(),/*deep=*/1);
    }
  this->CopyComponentNames( da );
}

//----------------------------------------------------------------------------
int vtkAbstractArray::CopyInformation(vtkInformation* infoFrom, int deep)
{
  // Copy all keys. NOTE: subclasses rely on this.
  vtkInformation* myInfo=this->GetInformation();
  myInfo->Copy(infoFrom,deep);

  // Remove any keys we own that are not to be copied here.
  // For now, remove per-component metadata.
  if (myInfo->Has(PER_COMPONENT()))
    {
    myInfo->Remove(PER_COMPONENT());
    }
  if (myInfo->Has(DISCRETE_VALUES()))
    {
    myInfo->Remove(DISCRETE_VALUES());
    }

  return 1;
}

//----------------------------------------------------------------------------
vtkInformation* vtkAbstractArray::GetInformation()
{
  if ( ! this->Information )
    {
    vtkInformation* info = vtkInformation::New();
    this->SetInformation( info );
    info->FastDelete();
    }
  return this->Information;
}

//----------------------------------------------------------------------------
template <class T>
int vtkAbstractArrayGetDataTypeSize(T*)
{
  return sizeof(T);
}

int vtkAbstractArray::GetDataTypeSize(int type)
{
  switch (type)
    {
    vtkTemplateMacro(
      return vtkAbstractArrayGetDataTypeSize(static_cast<VTK_TT*>(0))
      );

    case VTK_BIT:
      return 0;
      break;

    case VTK_STRING:
      return 0;
      break;

    case VTK_UNICODE_STRING:
      return 0;
      break;

    default:
      vtkGenericWarningMacro(<<"Unsupported data type!");
    }

  return 1;
}

// ----------------------------------------------------------------------
vtkAbstractArray* vtkAbstractArray::CreateArray(int dataType)
{
  switch (dataType)
    {
    case VTK_BIT:
      return vtkBitArray::New();

    case VTK_CHAR:
      return vtkCharArray::New();

    case VTK_SIGNED_CHAR:
      return vtkSignedCharArray::New();

    case VTK_UNSIGNED_CHAR:
      return vtkUnsignedCharArray::New();

    case VTK_SHORT:
      return vtkShortArray::New();

    case VTK_UNSIGNED_SHORT:
      return vtkUnsignedShortArray::New();

    case VTK_INT:
      return vtkIntArray::New();

    case VTK_UNSIGNED_INT:
      return vtkUnsignedIntArray::New();

    case VTK_LONG:
      return vtkLongArray::New();

    case VTK_UNSIGNED_LONG:
      return vtkUnsignedLongArray::New();

#if defined(VTK_TYPE_USE_LONG_LONG)
    case VTK_LONG_LONG:
      return vtkLongLongArray::New();

    case VTK_UNSIGNED_LONG_LONG:
      return vtkUnsignedLongLongArray::New();
#endif

#if defined(VTK_TYPE_USE___INT64)
    case VTK___INT64:
      return vtk__Int64Array::New();
      break;

# if defined(VTK_TYPE_CONVERT_UI64_TO_DOUBLE)
    case VTK_UNSIGNED___INT64:
      return vtkUnsigned__Int64Array::New();
      break;
# endif
#endif

    case VTK_FLOAT:
      return vtkFloatArray::New();

    case VTK_DOUBLE:
      return vtkDoubleArray::New();

    case VTK_ID_TYPE:
      return vtkIdTypeArray::New();

    case VTK_STRING:
      return vtkStringArray::New();

    case VTK_UNICODE_STRING:
      return vtkUnicodeStringArray::New();

    case VTK_VARIANT:
      return vtkVariantArray::New();

    default:
      break;
    }

  vtkGenericWarningMacro("Unsupported data type: " << dataType
                         << "! Setting to VTK_DOUBLE");
  return vtkDoubleArray::New();
}

//---------------------------------------------------------------------------
template <typename T>
vtkVariant vtkAbstractArrayGetVariantValue(T* arr, vtkIdType index)
{
  return vtkVariant(arr[index]);
}

//----------------------------------------------------------------------------
vtkVariant vtkAbstractArray::GetVariantValue(vtkIdType i)
{
  vtkVariant val;
  switch(this->GetDataType())
    {
    vtkExtraExtendedTemplateMacro(val = vtkAbstractArrayGetVariantValue(
      static_cast<VTK_TT*>(this->GetVoidPointer(0)), i));
    }
  return val;
}

//----------------------------------------------------------------------------
void vtkAbstractArray::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  const char* name = this->GetName();
  if (name)
    {
    os << indent << "Name: " << name << "\n";
    }
  else
    {
    os << indent << "Name: (none)\n";
    }
  os << indent << "Data type: " << this->GetDataTypeAsString() << "\n";
  os << indent << "Size: " << this->Size << "\n";
  os << indent << "MaxId: " << this->MaxId << "\n";
  os << indent << "NumberOfComponents: " << this->NumberOfComponents << endl;
  if ( this->ComponentNames )
    {
    os << indent << "ComponentNames: " << endl;
    vtkIndent nextIndent = indent.GetNextIndent();
    for ( unsigned int i=0; i < this->ComponentNames->size(); ++i )
      {
      os << nextIndent << i << " : " << this->ComponentNames->at(i) << endl;
      }
    }
  os << indent << "Information: " << this->Information << endl;
  if ( this->Information )
    {
    this->Information->PrintSelf( os, indent.GetNextIndent() );
    }
}

//--------------------------------------------------------------------------
void vtkAbstractArray::InsertVariantValue(vtkIdType id, vtkVariant value)
{
  if ( id >= this->Size )
    {
    int status = this->Resize(id+1);
    if (!status)
      {
      vtkErrorMacro(<<"FAILED to extend array to accommodate new ID "
                    << id);
      return;
      }
    }
  if ( id > this->MaxId )
    {
    this->MaxId = id;
    }
  this->SetVariantValue(id, value);
}

//--------------------------------------------------------------------------
void vtkAbstractArray::GetProminentComponentValues(
  int comp, vtkVariantArray* values,
  double uncertainty, double minimumProminence)
{
  if (!values || comp < -1 || comp >= this->NumberOfComponents)
    {
    return;
    }

  values->Initialize();
  values->SetNumberOfComponents(comp < 0 ? this->NumberOfComponents : 1);

  bool justCreated = false;
  vtkInformation* info = this->GetInformation();
  const double* lastParams = info ?
    (info->Has(DISCRETE_VALUE_SAMPLE_PARAMETERS()) ?
     info->Get(DISCRETE_VALUE_SAMPLE_PARAMETERS()) : 0) : 0;
  if (comp >= 0 && info)
    {
    vtkInformationVector* infoVec = info->Get(PER_COMPONENT());
    if (!infoVec || infoVec->GetNumberOfInformationObjects() < this->NumberOfComponents)
      {
      infoVec = vtkInformationVector::New();
      infoVec->SetNumberOfInformationObjects(this->NumberOfComponents);
      info->Set(PER_COMPONENT(), infoVec);
      infoVec->FastDelete();
      justCreated = true;
      }
    info = infoVec->GetInformationObject(comp);
    }
  if (info)
    {
    // Any insane parameter values map to
    // deterministic, exhaustive enumeration of all
    // distinct values:
    if (uncertainty < 0. || uncertainty > 1.)
      {
      uncertainty = 0.;
      }
    if (minimumProminence < 0. || minimumProminence > 1.)
      {
      minimumProminence = 0.;
      }
    // Are parameter values requesting more certainty in reporting or
    // that less-prominent values be reported? If so, recompute.
    bool tighterParams = lastParams ?
      (lastParams[0] > uncertainty ||
       lastParams[1] > minimumProminence ?
       true :
       false)
      : true;
    // Recompute discrete value set when the array has been
    // modified since the information was written.
    if (
      !info->Has(DISCRETE_VALUES()) || tighterParams ||
      this->GetMTime() > info->GetMTime() || justCreated)
      {
      this->UpdateDiscreteValueSet(uncertainty, minimumProminence);
      }
    }
  else
    {
    return;
    }

  vtkIdType len;
  const vtkVariant* vals = info->Get(DISCRETE_VALUES());
  if ((vals = info->Get(DISCRETE_VALUES())) != NULL)
    {
    len = info->Length(DISCRETE_VALUES());
    values->SetNumberOfTuples(len / values->GetNumberOfComponents());
    for (vtkIdType i = 0; i < len; ++i)
      {
      values->SetVariantValue(i, vals[i]);
      }
    }
}

//--------------------------------------------------------------------------
namespace
{
template<typename T>
bool AccumulateSampleValues(
  T* array, int nc, vtkIdType begin, vtkIdType end, vtkIdType vtkNotUsed(minFreq),
  std::vector<std::set<vtkVariant> >& uniques)
{
  // number of discrete components remaining (tracked during iteration):
  int ndc = nc;
  std::pair<std::set<vtkVariant>::iterator,bool> result;
  // Here we iterate over the components and add to their respective lists
  // of previously encountered values -- as long as there are not too many
  // values already in the list. We also accumulate each component's value
  // into a vtkVariantArray named tuple, which is added to the list of
  // unique vectors -- again assuming it is not already too long.
  for (vtkIdType i = begin; i < end && ndc; ++i)
    {
    // First, attempt a per-component insert.
    for (int j = 0; j < nc; ++ j)
      {
      if (uniques[j].size() > vtkAbstractArray::MAX_DISCRETE_VALUES)
        continue;
      result = uniques[j].insert(array[i * nc + j]);
      if (result.second)
        {
        if (uniques[j].size() == vtkAbstractArray::MAX_DISCRETE_VALUES + 1)
          {
          -- ndc;
          }
        }
      }
    // Now, as long as no component has exceeded MAX_DISCRETE_VALUES unique
    // values, it is worth seeing whether the tuple as a whole is unique:
    if ( nc > 1 && ndc == nc )
      {
      vtkNew<vtkVariantArray> tuple;
      tuple->SetNumberOfComponents(nc);
      tuple->SetNumberOfTuples(1);
      for (int j = 0; j < nc; ++j)
        {
        tuple->SetVariantValue(j, array[i * nc + j]);
        }
      vtkVariant wholeThing(tuple.GetPointer());
      result = uniques[nc].insert(wholeThing);
      }
    }
  return ndc == 0;
}
} // End anonymous namespace.

//--------------------------------------------------------------------------
void vtkAbstractArray::UpdateDiscreteValueSet(
  double uncertainty, double minimumProminence)
{
  // For an array with T tuples and given uncertainty U and mininumum
  // prominence P, we sample N blocks of M tuples each, with
  // M*N = f(T; P, U) and f some sublinear function of T.
  // If every component plus all components taken together each have more than
  // MAX_DISCRETE_VALUES distinct values, then we exit early.
  // M is chosen based on the number of bytes per tuple to maximize use of a cache
  // line (assuming a 64-byte cache line until kwsys::SystemInformation or the like
  // can provide a platform-independent way to query it).
  //
  // N is chosen to satisfy the requested uncertainty and prominence criteria specified.
#define VTK_CACHE_LINE_SIZE 64
#define VTK_SAMPLE_FACTOR 5
  int numberOfComponentsWithProminentValues = 0;
  int nc = this->NumberOfComponents;
  int blockSize = VTK_CACHE_LINE_SIZE / (this->GetDataTypeSize() * nc);
  if (!blockSize) blockSize = 4;
  double logfac = 1.;
  vtkIdType nt = this->GetNumberOfTuples();
  if (this->MaxId > 0)
    {
    logfac = -log(uncertainty * minimumProminence) / minimumProminence;
    if (logfac < 0)
      {
      logfac = -logfac;
      }
    }
  vtkIdType numberOfSampleTuples;
  if (vtkMath::IsInf(logfac))
    {
    numberOfSampleTuples = nt;
    }
  else
    {
    numberOfSampleTuples = VTK_SAMPLE_FACTOR * logfac;
    }
  vtkIdType minFreq = static_cast<vtkIdType>(
    numberOfSampleTuples * minimumProminence / 2);
  vtkIdType numberOfBlocks =
    numberOfSampleTuples / blockSize +
    (numberOfSampleTuples % blockSize ? 1 : 0);
  if (numberOfBlocks * blockSize < 2 * MAX_DISCRETE_VALUES)
    {
    numberOfBlocks =
      2 * MAX_DISCRETE_VALUES / blockSize +
      (2 * MAX_DISCRETE_VALUES % blockSize ? 1 : 0);
    }
  std::vector<std::set<vtkVariant> > uniques(nc > 1 ? nc + 1 : nc);
  if (numberOfBlocks * blockSize > this->MaxId / 2)
    { // Awwww, just do the whole array already!
    switch(this->GetDataType())
      {
      vtkSuperExtraExtendedTemplateMacro(
        AccumulateSampleValues(
          static_cast<VTK_TT*>(this->GetVoidPointer(0)),
          nc, 0, nt, minFreq, uniques));
    default:
      vtkErrorMacro("Array type " << this->GetClassName() << " not supported.");
      break;
      }
    }
  else
    { // Choose random blocks
    vtkNew<vtkMinimalStandardRandomSequence> seq;
    // test different blocks each time we're called:
    seq->SetSeed(seq->GetMTime() ^ 0xdeadbeef);
    vtkIdType totalBlockCount =
      nt / blockSize +
      (nt % blockSize ? 1 : 0);
    for (int i = 0; i < numberOfBlocks; ++ i, seq->Next())
      {
      vtkIdType startTuple =
        static_cast<vtkIdType>(seq->GetValue() * totalBlockCount) * blockSize;
      vtkIdType endTuple = startTuple + blockSize;
      endTuple = endTuple < nt ? endTuple : nt;
      bool endEarly;
      switch(this->GetDataType())
        {
        vtkSuperExtraExtendedTemplateMacro(
          endEarly = AccumulateSampleValues(
            static_cast<VTK_TT*>(this->GetVoidPointer(0)),
            nc, startTuple, endTuple, minFreq, uniques));
      default:
        vtkErrorMacro(
          "Array type " << this->GetClassName() << " not supported.");
        endEarly = true;
        break;
        }
      if (endEarly)
        break;
      }
    }
  int c;
  for (c = 0; c < nc; ++c)
    {
    if (uniques[c].size() <= MAX_DISCRETE_VALUES)
      {
      ++ numberOfComponentsWithProminentValues;
      std::vector<vtkVariant> compUniques;
      std::set<vtkVariant>::iterator sit;
      for (sit = uniques[c].begin(); sit != uniques[c].end(); ++sit)
        {
        compUniques.push_back(*sit);
        }
      this->GetInformation()->Get(
        PER_COMPONENT())->GetInformationObject(c)->Set(
        DISCRETE_VALUES(), &compUniques[0],
        static_cast<int>(compUniques.size()));
      }
    else
      {
      this->GetInformation()->Get(
        PER_COMPONENT())->GetInformationObject(c)->Remove(
        DISCRETE_VALUES());
      }
    }
  if (nc > 1 && uniques[nc].size() <= MAX_DISCRETE_VALUES)
    {
    std::vector<vtkVariant> compUniques;
    std::set<vtkVariant>::iterator sit;
    for (sit = uniques[nc].begin(); sit != uniques[nc].end(); ++sit)
      {
      vtkAbstractArray* tuple = sit->ToArray();
      if (tuple)
        {
        for (c = 0; c <= tuple->GetMaxId(); ++c)
          {
          compUniques.push_back(tuple->GetVariantValue(c));
          }
        }
      }
    ++ numberOfComponentsWithProminentValues;
    this->GetInformation()->Set(
      DISCRETE_VALUES(), &compUniques[0],
      static_cast<int>(compUniques.size()));
    }
  else
    { // Remove the key
    this->GetInformation()->Remove(DISCRETE_VALUES());
    }
  if (numberOfComponentsWithProminentValues > 0)
    {
    double params[2];
    params[0] = uncertainty;
    params[1] = minimumProminence;
    this->GetInformation()->Set(
      DISCRETE_VALUE_SAMPLE_PARAMETERS(), params, 2);
    }
  else
    { // Remove the parameter-value key
    this->GetInformation()->Remove(DISCRETE_VALUE_SAMPLE_PARAMETERS());
    }
}
