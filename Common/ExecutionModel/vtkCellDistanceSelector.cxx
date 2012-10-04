#include "vtkCellDistanceSelector.h"

#include <vtkCell.h>
#include <vtkCellData.h>
#include <vtkIdList.h>
#include <vtkObjectFactory.h>
#include <vtkPointData.h>
#include <vtkDataSet.h>
#include <vtkUnstructuredGrid.h>
#include <vtkStructuredGrid.h>
#include <vtkPolyData.h>
#include <vtkCellLinks.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkCompositeDataSet.h>
#include <vtkCompositeDataIterator.h>
#include <vtkSelection.h>
#include <vtkSelectionNode.h>
#include <vtkIdTypeArray.h>
#include <vtkSmartPointer.h>

#include <map>
#include <vector>

vtkStandardNewMacro(vtkCellDistanceSelector);

// ----------------------------------------------------------------------
vtkCellDistanceSelector::vtkCellDistanceSelector()
{
  this->Distance = 1;
  this->IncludeSeed = 1;
  this->AddIntermediate = 1;
  this->SetNumberOfInputPorts(2);
}

// ----------------------------------------------------------------------
vtkCellDistanceSelector::~vtkCellDistanceSelector()
{
}

// ----------------------------------------------------------------------
void vtkCellDistanceSelector::SetDataObjectConnection ( vtkAlgorithmOutput* in )
{
  this->SetInputConnection(1,in);
}

// ----------------------------------------------------------------------
void vtkCellDistanceSelector::PrintSelf( ostream& os, vtkIndent indent )
{
  this->Superclass::PrintSelf( os,indent );
}

// ----------------------------------------------------------------------
int vtkCellDistanceSelector::FillInputPortInformation( int port, vtkInformation* info )
{
  switch (port)
    {
    case 0:
      info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkSelection");
      break;
    case 1:
      info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(),
                "vtkCompositeDataSet");
      break;
    }
  //info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 0);
  return 1;
}

// ----------------------------------------------------------------------
void vtkCellDistanceSelector::AddSelectionNode( vtkSelection* output, 
                                                vtkSmartPointer<vtkDataArray> outIndices,
                                                int composite_index, int d )
{
  vtkSmartPointer<vtkSelectionNode> outSelNode = vtkSmartPointer<vtkSelectionNode>::New();
  outSelNode->SetContentType(vtkSelectionNode::INDICES);
  outSelNode->SetFieldType(vtkSelectionNode::CELL);
  outSelNode->GetProperties()->Set(vtkSelectionNode::COMPOSITE_INDEX(),composite_index);
  outSelNode->GetProperties()->Set(vtkSelectionNode::HIERARCHICAL_LEVEL(),d); // FIXME: Attention, la clé HIERARCHICAL_LEVEL est ici utilisée pour stocker la distance aux mailles d'origine
  outSelNode->SetSelectionList(outIndices);
  output->AddNode(outSelNode);

  vtkDebugMacro(<<"CellDistance: d="<<d<<", Ajout du node :\n");
  outSelNode->PrintSelf(cout,vtkIndent(2));
}

// ----------------------------------------------------------------------
int vtkCellDistanceSelector::RequestData( vtkInformation* vtkNotUsed( request ),
                                          vtkInformationVector** inputVector,
                                          vtkInformationVector* outputVector )
{
  vtkInformation *inSelectionInfo = inputVector[0]->GetInformationObject( 0 );
  vtkInformation *inDataObjectInfo = inputVector[1]->GetInformationObject( 0 );

  vtkInformation *outInfo = outputVector->GetInformationObject( 0 );

  vtkSelection* inputSelection = vtkSelection::SafeDownCast( inSelectionInfo->Get( vtkDataObject::DATA_OBJECT() ) );

  vtkCompositeDataSet * compositeInput = vtkCompositeDataSet::SafeDownCast( inDataObjectInfo->Get(vtkDataObject::DATA_OBJECT() ) );

  vtkSelection *output = vtkSelection::SafeDownCast(outInfo->Get( vtkDataObject::DATA_OBJECT() ) );

  if (compositeInput == 0)
    {
    vtkErrorMacro(<<"Missing input data object");
    return 0;
    }

  if ( inputSelection == 0)
    {
    vtkErrorMacro(<<"Missing input selection");
    return 0;
    }

  std::map<int,std::vector<vtkSelectionNode*> > partSelections;
  int nSelNodes = inputSelection->GetNumberOfNodes();
  for ( int i = 0; i < nSelNodes; ++ i )
    {
    vtkSelectionNode* sn = inputSelection->GetNode( i);
    int composite_index = sn->GetProperties()->Get(vtkSelectionNode::COMPOSITE_INDEX()) ;
    partSelections[composite_index].push_back(sn);
    }

  vtkCompositeDataIterator* inputIterator = compositeInput->NewIterator();
  inputIterator->SkipEmptyNodesOn();
  inputIterator->InitTraversal();
  inputIterator->GoToFirstItem();
  while ( inputIterator->IsDoneWithTraversal() == 0 )
    {
    vtkDataSet * input = vtkDataSet::SafeDownCast( inputIterator->GetCurrentDataObject() );
     // NB: composite indices start at 1
    int composite_index = inputIterator->GetCurrentFlatIndex();
    inputIterator->GoToNextItem();

    std::vector<vtkSelectionNode*>::iterator selNodeIt = partSelections[composite_index].begin();
    while ( selNodeIt != partSelections[composite_index].end() )
      {
      vtkSelectionNode* selectionNode = *selNodeIt;
      ++selNodeIt;

      vtkDataArray* selectionList = vtkDataArray::SafeDownCast( selectionNode->GetSelectionList() );
      vtkIdType numSeeds = selectionList->GetNumberOfTuples();
      if (numSeeds > 0
          && selectionNode->GetContentType() == vtkSelectionNode::INDICES
          && selectionNode->GetFieldType() == vtkSelectionNode::CELL
          && input->GetNumberOfCells()>0 )
        {
        vtkIdType numCells = input->GetNumberOfCells();

        vtkUnstructuredGrid* ug_input =
          vtkUnstructuredGrid::SafeDownCast( input);
        vtkStructuredGrid* sg_input = vtkStructuredGrid::SafeDownCast(
                                                                      input);
        vtkPolyData* pd_input = vtkPolyData::SafeDownCast( input);

        vtkCellLinks * links = 0;
        if (ug_input != 0)
          {
          if (ug_input->GetCellLinks() == 0)
            ug_input->BuildLinks();
          links = ug_input->GetCellLinks();
          }

        std::vector<int> flags(numCells,0);

        vtkSmartPointer<vtkIdTypeArray> outIndices = vtkSmartPointer<vtkIdTypeArray>::New();
        outIndices->SetNumberOfTuples(numSeeds);

        int seedCount = 0;
        for ( int i = 0; i < numSeeds; ++ i )
          {
          vtkIdType cellIndex = static_cast<vtkIdType> (selectionList->GetTuple1( i));
          if( cellIndex>=0 && cellIndex<numCells )
            {
            flags[cellIndex] = true;
            outIndices->SetTuple1(seedCount++,cellIndex);
            }
          else
            {
            vtkWarningMacro(<<"Selection's cell index out of bounds ("<<cellIndex<<"/"<<numCells<<")\n");
            }
          }
        outIndices->SetNumberOfTuples(seedCount);

        vtkSmartPointer<vtkIdTypeArray> finalIndices = vtkSmartPointer<vtkIdTypeArray>::New();
        vtkSmartPointer<vtkIntArray> cellDistance = vtkSmartPointer<vtkIntArray>::New();
        cellDistance->SetName("CellDistance");

        for ( int d = 0; d < this->Distance; d++)
          {
          vtkSmartPointer<vtkIdTypeArray> nextIndices = vtkSmartPointer<vtkIdTypeArray>::New();

          if (ug_input != 0)
            {
            int nIndices = outIndices->GetNumberOfTuples();
            for ( int i = 0; i < nIndices; ++ i )
              {
              vtkIdType cellIndex = static_cast<vtkIdType>( outIndices->GetTuple1( i ) );
              vtkIdType * points;
              vtkIdType n;
              ug_input->GetCellPoints(cellIndex, n, points);
              for ( int k = 0; k < n; ++ k )
                {
                vtkIdType pid = points[k];
                int np = links->GetNcells(pid);
                vtkIdType* cells = links->GetCells(pid);
                for ( int j = 0; j < np; j++)
                  {
                  vtkIdType cid = cells[j];
                  if( cid>=0 && cid<numCells )
                    {
                    if (!flags[cid])
                      {
                      flags[cid] = true;
                      nextIndices->InsertNextValue(cid);
                      }
                    }
                  else
                    {
                    vtkWarningMacro(<<"Selection's cell index out of bounds ("<<cid<<"/"<<numCells<<")\n");
                    }
                  }
                }
              }
            }
          else if ( pd_input != 0 )
            {
            pd_input->BuildLinks();
            int nIndices = outIndices->GetNumberOfTuples();
            for ( int i = 0; i < nIndices; ++ i )
              {
              vtkIdType cellIndex = static_cast<vtkIdType>( outIndices->GetTuple1( i) );
              vtkIdType* points;
              vtkIdType n;
              pd_input->GetCellPoints(cellIndex, n, points);
              for ( int k = 0; k < n; ++ k )
                {
                vtkIdType pid = points[k];
                short unsigned int np;
                vtkIdType* cells;
                pd_input->GetPointCells(pid, np, cells);
                for ( int j = 0; j < np; j++)
                  {
                  vtkIdType cid = cells[j];
                  if( cid>=0 && cid<numCells )
                    {
                    if (!flags[cid])
                      {
                      flags[cid] = true;
                      nextIndices->InsertNextValue(cid);
                      }
                    }
                  else
                    {
                    vtkWarningMacro(<<"Selection's cell index out of bounds ("<<cid<<"/"<<numCells<<")\n");
                    }
                  }
                }
              }
            }
          else if (sg_input != 0)
            {
            int dim[3];
            sg_input->GetDimensions(dim);
            dim[0]--;
            dim[1]--;
            dim[2]--;

            int nIndices = outIndices->GetNumberOfTuples();
            for ( int i = 0; i < nIndices; ++ i )
              {
              vtkIdType cellIndex = static_cast<vtkIdType>( outIndices->GetTuple1( i ) );
              vtkIdType cellId = cellIndex;
              vtkIdType ijk[3];
              for ( int c = 0; c < 3; c++)
                {
                if (dim[c] <= 1)
                  ijk[c] = 0;
                else
                  {
                  ijk[c] = cellId % dim[c];
                  cellId /= dim[c];
                  }
                }
              for ( int k = -1; k <= 1; k++)
                for ( int j = -1; j <= 1; j++)
                  for ( int l = -1; l <= 1; ++ l )
                    {
                    int I = ijk[0] + l, J = ijk[1] + j, K =
                      ijk[2] + k;
                    if ( i >= 0 && I < dim[0] && J >= 0 && J
                        < dim[1] && K >= 0 && K
                        < dim[2])
                      {
                      cellId = I + J * dim[0] + K
                        * dim[0] * dim[1];
                      if( cellId>=0 && cellId<numCells )
                        {
                        if (!flags[cellId])
                          {
                          flags[cellId] = true;
                          nextIndices->InsertNextValue( cellId );
                          }
                        }
                      else
                        {
                        vtkWarningMacro(<<"Selection's cell index out of bounds ("<<cellId<<"/"<<numCells<<")\n");
                        }
                      }
                    }
              }
            }
          else
            {
            vtkErrorMacro(<<"Unsupported data type : "<<input->GetClassName()<<"\n");
            }

          if( (d==0 && this->IncludeSeed) || (d>0 && this->AddIntermediate) )
            {
            int ni = outIndices->GetNumberOfTuples();
            for( int i=0;i<ni;++ i )
              {
              cellDistance->InsertNextTuple1( d );
              finalIndices->InsertNextTuple1( outIndices->GetTuple1( i ) );
              }
            }

          outIndices = nextIndices;
          } // for d in [0,Distance[

        if( (this->Distance==0 && this->IncludeSeed) || (this->Distance>0 && this->AddIntermediate) )
          {
          int ni = outIndices->GetNumberOfTuples();
          for( int i=0;i<ni;++ i )
            {
            cellDistance->InsertNextTuple1( this->Distance );
            finalIndices->InsertNextTuple1( outIndices->GetTuple1( i ) );
            }
          }

        if( finalIndices->GetNumberOfTuples() > 0 )
          {
          vtkSmartPointer<vtkSelectionNode> outSelNode = vtkSmartPointer<vtkSelectionNode>::New();
          outSelNode->SetContentType(vtkSelectionNode::INDICES);
          outSelNode->SetFieldType(vtkSelectionNode::CELL);
          outSelNode->GetProperties()->Set(vtkSelectionNode::COMPOSITE_INDEX(),composite_index);
          outSelNode->SetSelectionList(finalIndices);
          outSelNode->GetSelectionData()->AddArray(cellDistance);
          output->AddNode(outSelNode);
          }
        }
      }
    }

  return 1;
}
