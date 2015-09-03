//
// Created by jia chen on 9/2/15.
//

#include "vtkSegy3DReader.h"
#include <vtkSmartPointer.h>
#include <vtkObjectFactory.h>

vtkStandardNewMacro(vtkSegy3DReader);

void vtkSegy3DReader::PrintSelf(ostream &os, vtkIndent indent)
{
    Superclass::PrintSelf(os, indent);
}

vtkImageData* vtkSegy3DReader::GetImage(int ImageNumber)
{
    reader.LoadFromFile(FileName);

    image = vtkSmartPointer<vtkImageData>::New();
    if(!reader.ExportData3D(image))
        cout << "Failed to export 3D image from reader" << endl;

    return image;
}