#include "vtkLinearExtractionFilter.h"

#include <vtkObjectFactory.h>
#include <vtkCell.h>
#include <vtkDataSet.h>
#include <vtkDoubleArray.h>
#include <vtkIdTypeArray.h>
#include <vtkCellData.h>
#include <vtkPointData.h>
#include <vtkMath.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkSelection.h>
#include <vtkSelectionNode.h>
#include <vtkSmartPointer.h>
#include <vtkCompositeDataSet.h>
#include <vtkCompositeDataIterator.h>

#ifdef VTK_LOVE_EXTENSIONS
#include <vtkExtractBlockById.h>
#endif

#include <assert.h>
#include <vector>
#include <algorithm>

using namespace std;

vtkCxxRevisionMacro(vtkLinearExtractionFilter, "");
vtkStandardNewMacro(vtkLinearExtractionFilter);

//#undef vtkDebugMacro
//#define vtkDebugMacro( x ) std::cout<<": " x; std::cout.flush()

vtkLinearExtractionFilter::vtkLinearExtractionFilter ( )
{
  this->Tolerance = 0.0;
  this->SetStartPoint(0,0,0);
  this->SetEndPoint(1,1,1);
}	// vtkLinearExtractionFilter::vtkLinearExtractionFilter

vtkLinearExtractionFilter::~vtkLinearExtractionFilter ( )
{
}	// vtkLinearExtractionFilter::~vtkLinearExtractionFilter

void vtkLinearExtractionFilter::PrintSelf (ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os,indent);

  os << indent << "Point 1   : (" 
     << this->StartPoint[0] << ", " << this->StartPoint [1] << ", " << this->StartPoint [2]
     << ")" << "\n"
     << indent << "Point 2   : ("
     << this->EndPoint [0] << ", " << this->EndPoint [1] << ", " << this->EndPoint [2] << ")"
     << "\n"
     << indent << "Tolerance : " << this->Tolerance
     << "\n";
}	// vtkLinearExtractionFilter::PrintSelf

int vtkLinearExtractionFilter::FillInputPortInformation(int port, vtkInformation *info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(),
            "vtkCompositeDataSet");
  //info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 0);
  return 1;
}

int vtkLinearExtractionFilter::RequestData(
                                           vtkInformation *vtkNotUsed(request),
                                           vtkInformationVector **inputVector,
                                           vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and ouptut
  vtkCompositeDataSet *compositeInput = vtkCompositeDataSet::SafeDownCast(
                                                                          inInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkSelection *output = vtkSelection::SafeDownCast(
                                                    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  // preparation de l'output
  if (0 == output)
    {
    vtkErrorMacro(<<"vtkLinearExtractionFilter: filter does not have any output.");
    return 0;
    }	// if (0 == output)

  if (0 == compositeInput)
    {
    vtkErrorMacro(<<"vtkLinearExtractionFilter: filter does not have any input.");
    return 0;
    }	// if (0 == input)

  vtkSmartPointer<vtkCompositeDataIterator> inputIterator = vtkCompositeDataIterator::New();
  inputIterator->SetDataSet(compositeInput);
  inputIterator->VisitOnlyLeavesOn();
  inputIterator->SkipEmptyNodesOn();
  inputIterator->InitTraversal();
  inputIterator->GoToFirstItem();
  while (inputIterator->IsDoneWithTraversal() == 0)
    {
    vtkDataSet * input = vtkDataSet::SafeDownCast(
                                                  inputIterator->GetCurrentDataObject());

    int conmposite_index = inputIterator->GetCurrentFlatIndex(); // et oui, les composite_index commencent a 1 :(
#ifdef VTK_LOVE_EXTENSIONS
    int partNumber = vtkExtractBlockById::GetDataObjectBlockId(input);
#else
    int partNumber = -1;
#endif
    if( partNumber == -1 ) partNumber = conmposite_index - 1;

    inputIterator->GoToNextItem();

    vtkSmartPointer<vtkIdTypeArray> indices = vtkSmartPointer<vtkIdTypeArray>::New();

    this->RequestDataInternal( input, indices );

    vtkSmartPointer<vtkSelectionNode> outSelNode = vtkSelectionNode::New();
    outSelNode->SetContentType(vtkSelectionNode::INDICES);
    outSelNode->SetFieldType(vtkSelectionNode::CELL);
    outSelNode->GetProperties()->Set(vtkSelectionNode::COMPOSITE_INDEX(),partNumber+1);
    outSelNode->SetSelectionList(indices);
    output->AddNode(outSelNode);
    //vtkDebugMacro(<<"vtkLinearExtractionFilter: Ajout du node :\n");
    //outSelNode->PrintSelf(cout,vtkIndent(2));
    }

  return 1;
}

void vtkLinearExtractionFilter::RequestDataInternal ( vtkDataSet* input, vtkIdTypeArray* outIndices )
{
  // La liste des données retenues / distance à P1 :
  vector< pair<vtkIdType, double> >	keptData;
  const vtkIdType	cellNum	= input->GetNumberOfCells ( );
  for (vtkIdType id = 0; id < cellNum; id++)
    {
    vtkCell*	cell	= input->GetCell (id);
    if (0 != cell)
      {
      // Coordonnée paramétrique du point intersécant sur la ligne :
      double	coords [3], pcoords [3];
      double t = 0;
      int						subId	= 0;

      // IntersectWithLine : intersection avec un segment et non avec une
      // ligne.
      if (0 != cell->IntersectWithLine (
                                        this->StartPoint, this->EndPoint, this->Tolerance, t, coords, pcoords,
                                        subId))
        {
        outIndices->InsertNextValue(id);
        }
      }	// if (0 != cell)
    }	// for (vtkIdType id = 0; id < cellNum; id++)

}	// vtkLinearExtractionFilter::ExecuteDistanceSorted
