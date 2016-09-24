//
// Created by jia chen on 9/2/15.
//

#include "vtkSegy2DReader.h"

#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkObjectFactory.h>

vtkStandardNewMacro(vtkSegy2DReader);

vtkSegy2DReader::vtkSegy2DReader()
{
    this->SetFileName(0);
    this->SetNumberOfInputPorts( 0 );
}

vtkSegy2DReader::~vtkSegy2DReader()
{
    this->SetFileName(0);
}

int vtkSegy2DReader::RequestData(vtkInformation* vtkNotUsed(request),
                                  vtkInformationVector** inputVector,
                                  vtkInformationVector* outputVector)
{
    vtkInformation* outInfo = outputVector->GetInformationObject(0);
    vtkPolyData* output = vtkPolyData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

    if (!this->FileName)
    {
        vtkErrorMacro(<<"A File Name must be specified.");
        return 0;
    }

    reader.LoadFromFile(FileName);
    cout << "FileName : " <<  FileName << endl;
    if(!reader.ExportData2D(output))
        cout << "Failed to request data for vtkSegy2DReader" << endl;
}

void vtkSegy2DReader::PrintSelf(ostream &os, vtkIndent indent)
{
    Superclass::PrintSelf(os, indent);
}
