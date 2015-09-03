//
// Created by jia chen on 9/2/15.
//

#ifndef SEGYVISUALIZER2D_VTKSEGY2DREADER_H
#define SEGYVISUALIZER2D_VTKSEGY2DREADER_H

#include "vtkSegy2DReader.h"

#include <vtkPolyDataAlgorithm.h>
#include "SegyReader.h"

class vtkSegy2DReader : public vtkPolyDataAlgorithm
{
public:
    static vtkSegy2DReader* New();
    vtkTypeMacro(vtkSegy2DReader,vtkPolyDataAlgorithm);
    void PrintSelf(ostream& os, vtkIndent indent);

    vtkSetStringMacro(FileName);
    vtkSegy2DReader(){}
protected:
    int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
                    vtkInformationVector* outputVector);
private:
    char *FileName;
    SegyReader reader;

private:
    vtkSegy2DReader(const vtkSegy2DReader&);  // Not implemented.
    void operator=(const vtkSegy2DReader&);  // Not implemented.


};


#endif //SEGYVISUALIZER2D_VTKSEGY2DREADER_H
