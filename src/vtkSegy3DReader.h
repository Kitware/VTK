//
// Created by jia chen on 9/2/15.
//

#ifndef SEGYVISUALIZER2D_VTKSEGY3DREADER_H
#define SEGYVISUALIZER2D_VTKSEGY3DREADER_H

#include <vtkImageAlgorithm.h>
#include <vtkImageData.h>
#include <vtkSmartPointer.h>
#include "SegyReader.h"

class vtkSegy3DReader : public vtkImageAlgorithm
{
public:
    static vtkSegy3DReader* New();

    vtkTypeMacro(vtkSegy3DReader,vtkImageAlgorithm);
    void PrintSelf(ostream& os, vtkIndent indent);

    vtkSetStringMacro(FileName);
    vtkGetStringMacro(FileName);

    virtual vtkImageData *GetImage(int ImageNumber);
private:
    char *FileName;
    SegyReader reader;
    vtkSegy3DReader(){}
    vtkSmartPointer<vtkImageData> image;
};


#endif //SEGYVISUALIZER2D_VTKSEGY3DREADER_H
