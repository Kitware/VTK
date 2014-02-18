/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMultiThreshold.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/

#include "vtkMultiThreshold.h"

#include <math.h>

#include "vtkCellData.h"
#include "vtkDataArray.h"
#include "vtkDataSet.h"
#include "vtkGenericCell.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPointSet.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkUnstructuredGrid.h"

vtkStandardNewMacro(vtkMultiThreshold);

// Prevent lots of error messages on the inner loop of the filter by keeping track of how many we have:
// (Note that this can fail in exquisitely confusing ways with multiple threads)
static int vtkMultiThresholdLimitErrorCount = 0;

static const char* vtkMultiThresholdSetOperationNames[] = {
  "AND",
  "OR",
  "XOR",
  "WOR",
  "NAND"
};

// Note the inverted ordering... index this with (-component-1) since the enum values are negative.
static const char* vtkMultiThresholdNormNames[] =
{
  "L1Norm",
  "L2Norm",
  "LInfinityNorm",
};

static double vtkMultiThresholdSingleComponentNorm( vtkDataArray* arr, vtkIdType tuple, int component )
{
  return arr->GetTuple( tuple )[component];
}

static double vtkMultiThresholdL1ComponentNorm( vtkDataArray* arr, vtkIdType tuple, int vtkNotUsed(component) )
{
  double* x = arr->GetTuple( tuple );
  double norm = 0.;
  int nc = arr->GetNumberOfComponents();
  for ( int i = 0; i < nc; ++i )
    {
    norm += fabs(x[i]);
    }
  return norm;
}

static double vtkMultiThresholdL2ComponentNorm( vtkDataArray* arr, vtkIdType tuple, int vtkNotUsed(component) )
{
  double* x = arr->GetTuple( tuple );
  double norm = 0.;
  int nc = arr->GetNumberOfComponents();
  for ( int i = 0; i < nc; ++i )
    {
    norm += x[i] * x[i];
    }
  return sqrt(norm);
}

static double vtkMultiThresholdLinfComponentNorm( vtkDataArray* arr, vtkIdType tuple, int vtkNotUsed(component) )
{
  double* x = arr->GetTuple( tuple );
  double norm = 0.;
  double xabs;
  int nc = arr->GetNumberOfComponents();
  for ( int i = 0; i < nc; ++i )
    {
    xabs = fabs( x[i] );
    if ( xabs > norm )
      norm = xabs;
    }
  return norm;
}

void vtkMultiThreshold::NormKey::ComputeNorm( vtkIdType cellId, vtkCell* cell, vtkDataArray* array, double cellNorm[2] ) const
{
  if ( ! array )
    {
    cellNorm[0] = cellNorm[1] = vtkMath::Nan();
    return;
    }

  if ( this->Association == vtkDataObject::FIELD_ASSOCIATION_POINTS )
    {
    vtkIdList* ptIds = cell->GetPointIds();
    cellNorm[0] = cellNorm[1] = this->NormFunction( array, ptIds->GetId( 0 ), this->Component );
    for ( int p = 1; p < cell->GetNumberOfPoints(); ++p )
      {
      double x = this->NormFunction( array, ptIds->GetId( p ), this->Component );
      if ( x < cellNorm[0] )
        cellNorm[0] = x;
      else if ( x > cellNorm[1] )
        cellNorm[1] = x;
      }
    }
  else
    {
    cellNorm[0] = this->NormFunction( array, cellId, this->Component );
    }
}

void vtkMultiThreshold::Set::PrintNodeName( ostream& os )
{
  os << "set" << this->Id;
}

#define VTK_MULTITHRESH_ABOVE_BOTTOM( val ) \
  (this->EndpointClosures[0] == CLOSED ? (val) >= this->EndpointValues[0] : (val) > this->EndpointValues[0])

#define VTK_MULTITHRESH_BELOW_TOP( val ) \
  (this->EndpointClosures[1] == CLOSED ? (val) <= this->EndpointValues[1] : (val) < this->EndpointValues[1])

#define VTK_MULTITHRESH_ABOVE_TOP( val ) \
  (this->EndpointClosures[1] == CLOSED ? (val) > this->EndpointValues[1] : (val) >= this->EndpointValues[1])

#define VTK_MULTITHRESH_BELOW_BOTTOM( val ) \
  (this->EndpointClosures[0] == CLOSED ? (val) < this->EndpointValues[0] : (val) <= this->EndpointValues[0])

#define VTK_MULTITHRESH_IS_IN_INTERVAL( val ) \
  ( VTK_MULTITHRESH_ABOVE_BOTTOM( val ) && VTK_MULTITHRESH_BELOW_TOP( val ) )

#define VTK_MULTITHRESH_SPANS_INTERVAL( val1, val2 ) \
  ( VTK_MULTITHRESH_ABOVE_TOP( val2 ) && VTK_MULTITHRESH_BELOW_BOTTOM( val1 ) )

int vtkMultiThreshold::Interval::Match( double cellNorm[2] )
{
  if ( this->Norm.Association == vtkDataObject::FIELD_ASSOCIATION_POINTS )
    {
    if ( this->Norm.AllScalars )
      {
      return VTK_MULTITHRESH_IS_IN_INTERVAL( cellNorm[0] ) && VTK_MULTITHRESH_IS_IN_INTERVAL( cellNorm[1] );
      }
    else
      {
      return
        ( VTK_MULTITHRESH_IS_IN_INTERVAL( cellNorm[0] ) ||
          VTK_MULTITHRESH_IS_IN_INTERVAL( cellNorm[1] ) ||
          VTK_MULTITHRESH_SPANS_INTERVAL( cellNorm[0], cellNorm[1] ) );
      }
    }
  else
    {
    return VTK_MULTITHRESH_IS_IN_INTERVAL( cellNorm[0] );
    }
}

void vtkMultiThreshold::Interval::PrintNode( ostream& os )
{
  os << "  set" << this->Id << " [shape=rect,";
  if ( this->OutputId >= 0 )
    {
    os << "style=filled,";
    }
  os << "label=\"";
  if ( this->Norm.Component < 0 )
    {
    os << vtkMultiThresholdNormNames[-this->Norm.Component-1] << "(";
    }
  os << ( this->Norm.Association == vtkDataObject::FIELD_ASSOCIATION_POINTS ? "point " : "cell " );
  if ( this->Norm.Type >= 0 )
    {
    os << vtkDataSetAttributes::GetAttributeTypeAsString(this->Norm.Type);
    }
  else
    {
    os << this->Norm.Name.c_str();
    }
  if ( this->Norm.Component < 0 )
    {
    os << ")";
    }
  else
    {
    os << "(" << this->Norm.Component << ")";
    }
  os << " in " << (this->EndpointClosures[0] == OPEN ? "]" : "[")
     << this->EndpointValues[0] << "," << this->EndpointValues[1]
     << (this->EndpointClosures[1] == OPEN ? "[" : "]")
     << "\"]" << endl;
}

void vtkMultiThreshold::BooleanSet::PrintNode( ostream& os )
{
  os << "  set" << this->Id << " [shape=rect,";
  if ( this->OutputId >= 0 )
    {
    os << "style=filled,";
    }
  os << "label=\"" << vtkMultiThresholdSetOperationNames[this->Operator] << "\"]" << endl;
}

void vtkMultiThreshold::PrintSelf( ostream& os, vtkIndent indent )
{
  this->Superclass::PrintSelf( os, indent );
  os << indent << "IntervalRules: " << this->IntervalRules.size() << endl;
  os << indent << "Sets: " << this->Sets.size() << " entries" << endl;
  os << indent << "DependentSets: " << this->DependentSets.size() << endl;
  os << indent << "NumberOfOutputs: " << this->NumberOfOutputs << endl;
  os << indent << "NextArrayIndex: " << this->NextArrayIndex << endl;
  this->PrintGraph( os );
}

int vtkMultiThreshold::AddIntervalSet(
  double xmin, double xmax, int omin, int omax,
  int assoc, const char* arrayName, int component, int allScalars )
{
  if ( ! arrayName )
    {
    vtkWarningMacro( "You passed a null array name." );
    return -1;
    }

  NormKey nk;

  nk.Association = assoc;
  nk.Type = -1;
  nk.Name = std::string( arrayName );
  nk.Component = component;
  nk.AllScalars = allScalars;

  return this->AddIntervalSet( nk, xmin, xmax, omin, omax );
}

int vtkMultiThreshold::AddIntervalSet(
  double xmin, double xmax, int omin, int omax,
  int assoc, int attribType, int component, int allScalars )
{
  if ( attribType < 0 || attribType >= vtkDataSetAttributes::NUM_ATTRIBUTES )
    {
    vtkWarningMacro( "You passed an invalid attribute type (" << attribType << ")" );
    return -1;
    }

  NormKey nk;

  nk.Association = assoc;
  nk.Type = attribType;
  nk.Component = component;
  nk.AllScalars = allScalars;

  return this->AddIntervalSet( nk, xmin, xmax, omin, omax );
}

int vtkMultiThreshold::AddBooleanSet( int operation, int numInputs, int* inputs )
{
  if ( operation < vtkMultiThreshold::AND || operation > vtkMultiThreshold::NAND )
    {
    vtkErrorMacro( "Invalid operation (" << operation << ")" );
    return -1;
    }

  if ( numInputs < 1 )
    {
    vtkErrorMacro( "Operators require at least one operand. You passed " << numInputs << "." );
    return -1;
    }

  int sId = (int)this->Sets.size();
  int i;
  for ( i = 0; i < numInputs; ++i )
    {
    int inId = inputs[i];
    if ( inId < 0 || inId >= sId )
      {
      vtkErrorMacro( "Input " << i << " is invalid (" << inId << ")." );
      return -1;
      }
    }

  BooleanSet* bset = new BooleanSet( sId, operation, inputs, inputs + numInputs );
  this->Sets.push_back( bset );
  this->DependentSets.push_back( TruthTreeValues() );

  // Add dependency to input sets
  for ( i = 0; i < numInputs; ++i )
    {
    int inId = inputs[i];
    this->DependentSets[inId].push_back( sId );
    }

  return sId;
}

int vtkMultiThreshold::OutputSet( int setId )
{
  if ( setId < 0 || setId >= (int) this->Sets.size() )
    {
    vtkWarningMacro( "Cannot output " << setId << " because there is no set with that label." );
    return -1;
    }

  if ( this->Sets[setId]->OutputId >= 0 )
    {
    // The set is already output. Don't complain, just pass the existing output ID.
    return this->Sets[setId]->OutputId;
    }

  this->Sets[setId]->OutputId = this->NumberOfOutputs++;
  this->Modified();
  return this->Sets[setId]->OutputId;
}

void vtkMultiThreshold::Reset()
{
  for ( std::vector<Set*>::iterator it = this->Sets.begin(); it != this->Sets.end(); ++it )
    {
    delete (*it);
    }
  this->Sets.clear();
  this->DependentSets.clear();
  this->IntervalRules.clear();
  this->NextArrayIndex = 0;
  this->NumberOfOutputs = 0;
}

vtkMultiThreshold::vtkMultiThreshold()
{
  this->NextArrayIndex = 0;
  this->NumberOfOutputs = 0;
}

vtkMultiThreshold::~vtkMultiThreshold()
{
  // This will delete all the interval and boolean sets:
  this->Reset();
}

int vtkMultiThreshold::AddIntervalSet( NormKey& nk, double xmin, double xmax, int omin, int omax )
{
  if ( xmin > xmax )
    {
    vtkWarningMacro( "Intervals must be specified with ascending values (xmin <= xmax)" );
    return -1;
    }

  // The following condition will only hold if xmin or xmax is a NAN (or maybe
  // if they are both infinite, which is an empty range anyway).
  if (!(xmin <= xmax) && !(xmin >= xmax))
    {
    vtkWarningMacro( "One of the interval endpoints is not a number." );
    return -1;
    }

  if ( xmin == xmax && ( omin == OPEN || omax == OPEN ) )
    {
    vtkWarningMacro( "An open interval with equal endpoints will always be empty. I won't help you waste my time." );
    return -1;
    }

  if ( nk.Association != vtkDataObject::FIELD_ASSOCIATION_POINTS && nk.Association != vtkDataObject::FIELD_ASSOCIATION_CELLS )
    {
    vtkWarningMacro( "You must pass FIELD_ASSOCIATION_POINTS or FIELD_ASSOCIATION_CELLS for the association." );
    return -1;
    }

  RuleMap::iterator normRules( this->IntervalRules.find( nk ) );
  if ( normRules == this->IntervalRules.end() )
    {
    nk.InputArrayIndex = this->NextArrayIndex++;
    if ( nk.Type == -1 )
      {
      this->SetInputArrayToProcess( nk.InputArrayIndex, 0, 0, nk.Association, nk.Name.c_str() );
      }
    else
      {
      this->SetInputArrayToProcess( nk.InputArrayIndex, 0, 0, nk.Association, nk.Type );
      }
    }
  else
    {
    nk.InputArrayIndex = normRules->first.InputArrayIndex;
    }

  vtkMultiThreshold::Interval* interval = new vtkMultiThreshold::Interval;
  interval->Norm = nk;
  interval->EndpointValues[0] = xmin;
  interval->EndpointValues[1] = xmax;
  interval->EndpointClosures[0] = omin;
  interval->EndpointClosures[1] = omax;
  if ( nk.Component >= 0 )
    {
    nk.NormFunction = interval->Norm.NormFunction = vtkMultiThresholdSingleComponentNorm;
    }
  else if ( nk.Component == -1 )
    {
    nk.NormFunction = interval->Norm.NormFunction = vtkMultiThresholdL1ComponentNorm;
    }
  else if ( nk.Component == -2 )
    {
    nk.NormFunction = interval->Norm.NormFunction = vtkMultiThresholdL2ComponentNorm;
    }
  else // ( nk.Component == -3 )
    {
    nk.NormFunction = interval->Norm.NormFunction = vtkMultiThresholdLinfComponentNorm;
    }

  int entry = (int)this->Sets.size();
  interval->Id = entry;

  this->Sets.push_back( interval );
  this->DependentSets.push_back( TruthTreeValues() );
  this->IntervalRules[ nk ].push_back( interval );

  return entry;
}

// User adds intervals
//   - as intervals are added, a unique list of the (assoc/array/comp) to which they refer is updated
//   - intervals are sorted by the (assoc/array/comp) id
// Optionally, user denotes operations between intervals sharing a common output

int vtkMultiThreshold::RequestData(
  vtkInformation* vtkNotUsed(req),
  vtkInformationVector** inputs,
  vtkInformationVector* output )
{
  int i;
  if ( this->Sets.size() == 0 )
    {
    // No rules to apply. Produce empty output.
    return 1;
    }

  // Reset error count so that each RequestData pass will generate at most 5 error messages.
  vtkMultiThresholdLimitErrorCount = 0;

  // I. Create multiblock output and 1 child dataset for each set to be output
  vtkInformation* iinfo = inputs[0]->GetInformationObject( 0 );
  vtkInformation* oinfo = output->GetInformationObject( 0 );

  int updateNumPieces =
    oinfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());
  int updatePiece =
    oinfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());

  vtkPointSet* in =
    vtkPointSet::SafeDownCast( iinfo->Get( vtkDataObject::DATA_OBJECT() ) );
  vtkMultiBlockDataSet* omesh =
    vtkMultiBlockDataSet::SafeDownCast(
      oinfo->Get( vtkDataObject::DATA_OBJECT() ) );
  if ( ! omesh )
    {
    return 0;
    }
  omesh->SetNumberOfBlocks(this->NumberOfOutputs);

  std::vector<vtkUnstructuredGrid*> outv; // vector of output datasets
  vtkUnstructuredGrid* ds;
  for ( i = 0; i < this->NumberOfOutputs; ++i )
    {
    vtkMultiBlockDataSet* block = vtkMultiBlockDataSet::New();
    omesh->SetBlock(i, block);
    block->Delete();

    block->SetNumberOfBlocks(updateNumPieces);
    ds = vtkUnstructuredGrid::New();
    ds->SetPoints( in->GetPoints() );
    ds->GetPointData()->PassData( in->GetPointData() );
    ds->GetCellData()->CopyGlobalIdsOn();
    ds->GetCellData()->CopyAllocate( in->GetCellData() );

    block->SetBlock(updatePiece, ds );
    ds->FastDelete();

    outv.push_back( ds );
    }

  // II. Prepare to loop over all the cells.
  //     A. Create a vector that we'll copy into setStates each time we start processing a new cell.
  //        Creating this summary ahead of time saves a lot of work in the big loop.
  vtkIdType inCell;
  vtkCellData* inCellData = in->GetCellData();

  std::set<int> unresolvedOutputs;
  // setStates is a vector of the same length as this->Sets.
  // Entries are INCONCLUSIVE, INCLUDE, or EXCLUDE for each interval set, and
  // some number between 0 and the number of entries in DependentSets[i] for each boolean set.
  // Since we have to reset setStates for each cell in the input mesh, we precompute its initial state as setStatesInit.
  TruthTreeValues setStates;
  TruthTreeValues setStatesInit;
  for ( i = 0; i < (int)this->Sets.size(); ++i )
    {
    BooleanSet* bset = this->Sets[i]->GetBooleanSetPointer();
    if ( ! bset )
      {
      setStatesInit.push_back( INCONCLUSIVE );
      }
    else
      {
      setStatesInit.push_back( (int)bset->Inputs.size() );
      }
    }

  // II. B. Verify that the requested input arrays exist on the inputs now that we have an input
  RuleMap::iterator aacn; // aacn = (association, attribute, component, norm)
  std::vector<vtkDataArray*> NormArrays;
  i = 0;
  for ( aacn = this->IntervalRules.begin(); aacn != this->IntervalRules.end(); ++aacn, ++i )
    {
    vtkDataArray* arr = this->GetInputArrayToProcess( aacn->first.InputArrayIndex, inputs );
    if ( ! arr )
      {
      vtkErrorMacro( "Input array for norm " << i << " is null" );
      return 0;
      }
    NormArrays.push_back( arr );
    }


  // II. C. Keep a generic cell handy for when we need to copy the input to the output
  vtkGenericCell* cell = vtkGenericCell::New();

  // III. Loop over each cell, copying it to output meshes as required. The stategy here is:
  //      For each cell C_i in the mesh,
  //         setStates <- setStatesInit
  //         unresolvedOutputs <- unresolvedOutputsInit
  //         For each (association,array,norm/component) named N_j
  //            Compute N_j for C_i, store in cellNorm[0] (& cellNorm[1] if attribute varies over cell)
  //            For each interval I_k defined over N_j,
  //               If I_k and cellNorm overlap appropriately (depending on N_j.AllScalars),
  //                  decision <- true
  //               else
  //                  decision <- false
  //               If I_k is an output
  //                  If decision is true
  //                     Copy C_i to output associated with I_k
  //                  Remove output associated with I_k from unresolvedOutputs
  //               Update sets whose values are dependent on the decision for I_k (recursively)
  //       All loops except the outermost will terminate early if unresolvedOutputs is empty.

  // For each cell in the mesh:
  for ( inCell = 0; inCell < in->GetNumberOfCells(); ++inCell )
    {
    in->GetCell( inCell, cell );
    unresolvedOutputs.clear();
    for ( i = 0; i < this->NumberOfOutputs; ++i )
      {
      unresolvedOutputs.insert( i );
      }
    setStates = setStatesInit;

    // For each norm of an attribute defined over the mesh:
    int normIdx = 0;
    for ( aacn = this->IntervalRules.begin(); (unresolvedOutputs.size()) && (aacn != this->IntervalRules.end()); ++aacn, ++normIdx )
      {
      double cellNorm[2]; // min,max used if aacn is a point array. otherwise, just min is used.
      Interval* ival;
      int iival;
      aacn->first.ComputeNorm( inCell, cell, NormArrays[normIdx], cellNorm );

      // For each interval test associated with the current norm:
      for ( iival = 0; (unresolvedOutputs.size()) && (iival < (int)aacn->second.size()); ++iival )
        {
        ival = aacn->second[iival];
        // See if the intervals overlap properly
        int match = ival->Match( cellNorm );
#if 0
        fprintf( stderr, "Cell %5llu [%10.4f,%10.4f] in [%10.4f,%10.4f]: %d\n",
          inCell, cellNorm[0], cellNorm[1], ival->EndpointValues[0], ival->EndpointValues[1], match );
#endif // 0
        setStates[ival->Id] = match ? INCLUDE : EXCLUDE;
        if ( ival->OutputId >= 0 )
          {
          if ( match )
            {
            // Note that we could eliminate points not referenced in the output meshes as we go,
            // but that's an optimization for later. Don't forget to modify UpdateDependents as well
            // if you do this.

            // copy cell to output
            vtkIdType outCell = outv[ival->OutputId]->InsertNextCell( cell->GetCellType(), cell->GetPointIds() );
            // copy cell data to output
            outv[ival->OutputId]->GetCellData()->CopyData( inCellData, inCell, outCell );
            }
          unresolvedOutputs.erase( ival->OutputId );
          }
        this->UpdateDependents( ival->Id, unresolvedOutputs, setStates, inCellData, inCell, cell, outv );
        } // ival
      }

#if 0
    fprintf( stderr, "Cell %5lld [", inCell );
    for ( i = 0; i < (int)this->Sets.size(); ++i )
      {
      fprintf( stderr, "%d", setStates[i] == INCLUDE ? 1 : 0 );
      }
    fprintf( stderr, "]\n" );
#endif // 0
    }

  cell->Delete();

  return 1;
}

int vtkMultiThreshold::FillInputPortInformation( int vtkNotUsed(port), vtkInformation *info )
{
  info->Set( vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPointSet" );
  return 1;
}

void vtkMultiThreshold::UpdateDependents(
  int id, std::set<int>& unresolvedOutputs, TruthTreeValues& setStates,
  vtkCellData* inCellData, vtkIdType inCell, vtkGenericCell* cell, std::vector<vtkUnstructuredGrid*>& outv )
{
  int lastMatch = setStates[id];
  // See if we can take care of boolean sets now.
  for ( TruthTreeValues::iterator dit = this->DependentSets[id].begin(); dit != this->DependentSets[id].end(); ++dit )
    {
    BooleanSet* bset = this->Sets[*dit]->GetBooleanSetPointer();
    if ( ! bset )
      {
      if ( ++vtkMultiThresholdLimitErrorCount > 5 )
        {
        vtkErrorMacro( "Set " << id << " has a dependent set (" << (*dit) << ") that isn't boolean. Results will suffer." );
        }
      }

    // If this dependent state has already been handled (i.e., is INCLUDE or EXCLUDE), skip the rest of the loop
    if ( setStates[bset->Id] < INCONCLUSIVE )
      continue;

    int decision = INCONCLUSIVE;
    switch ( bset->Operator )
      {
    case AND:
      if ( lastMatch == EXCLUDE )
        { // an input is false => output is false
        decision = EXCLUDE;
        }
      else
        {
        if ( --setStates[*dit] == 0 )
          { // we just checked the last input and it's true
          decision = INCLUDE;
          }
        }
      break;
    case OR:
      if ( lastMatch == INCLUDE )
        { // any input is true => output is true
        decision = INCLUDE;
        }
      else
        {
        if ( --setStates[*dit] == 0 )
          { // we just checked the last input and they're all false
          decision = EXCLUDE;
          }
        }
      break;
    case XOR:
      if ( --setStates[*dit] == 0 )
        { // we just checked the last input... only now can we determine the output
        int cnt = 0;
        for ( TruthTreeValues::iterator inIt = bset->Inputs.begin(); inIt != bset->Inputs.end(); ++inIt )
          {
          if ( setStates[*inIt] == INCLUDE )
            {
            ++cnt;
            }
          else if ( setStates[*inIt] != EXCLUDE )
            {
            if ( ++vtkMultiThresholdLimitErrorCount > 5 )
              {
              vtkErrorMacro( "Boolean set " << (*dit) << " (XOR) had indeterminate input (" << (*inIt) << ") on final pass" );
              }
            }
          }
        decision = cnt == 1 ? INCLUDE : EXCLUDE;
        }
      break;
    case WOR:
      if ( --setStates[*dit] == 0 )
        { // we just checked the last input... only now can we determine the output
        int cnt = 0;
        for ( TruthTreeValues::iterator inIt = bset->Inputs.begin(); inIt != bset->Inputs.end(); ++inIt )
          {
          if ( setStates[*inIt] == INCLUDE )
            {
            ++cnt;
            }
          else if ( setStates[*inIt] != EXCLUDE )
            {
            if ( ++vtkMultiThresholdLimitErrorCount > 5 )
              {
              vtkErrorMacro( "Boolean set " << (*dit) << " (WOR) had indeterminate input (" << (*inIt) << ") on final pass" );
              }
            }
          }
        decision = cnt % 2 ? INCLUDE : EXCLUDE;
        }
      break;
    case NAND:
      if ( lastMatch == EXCLUDE )
        { // an input is false => output is true
        decision = INCLUDE;
        }
      else
        {
        if ( --setStates[*dit] == 0 )
          { // we just checked the last input and it's true
          decision = EXCLUDE;
          }
        }
      break;
    default:
      // Do nothing
      break;
      }

    if ( decision < INCONCLUSIVE )
      {
      setStates[*dit] = decision;
      if ( bset->OutputId >= 0 )
        {
        if ( decision == INCLUDE )
          {
          // copy cell to output
          vtkIdType outCell = outv[bset->OutputId]->InsertNextCell( cell->GetCellType(), cell->GetPointIds() );
          // copy cell data to output
          outv[bset->OutputId]->GetCellData()->CopyData( inCellData, inCell, outCell );
          }
        unresolvedOutputs.erase( bset->OutputId );
        }
      if ( unresolvedOutputs.size() )
        { // ignore parts of the graph that will not influence output
        this->UpdateDependents( *dit, unresolvedOutputs, setStates, inCellData, inCell, cell, outv );
        }
      }
    }
}

void vtkMultiThreshold::PrintGraph( ostream& os )
{
  os << "digraph MultiThreshold {" << endl;
  for ( std::vector<Set*>::iterator it = this->Sets.begin(); it != this->Sets.end(); ++it )
    {
    (*it)->PrintNode( os );
    }
  int ds = 0;
  for ( TruthTree::iterator dit = this->DependentSets.begin(); dit != this->DependentSets.end(); ++dit, ++ds )
    {
    for ( TruthTreeValues::iterator vit = dit->begin(); vit != dit->end(); ++vit )
      {
      os << "  ";
      this->Sets[ds]->PrintNodeName(os);
      os << " -> ";
      this->Sets[*vit]->PrintNodeName(os);
      os << endl;
      }
    }
  os << "}" << endl;
}
