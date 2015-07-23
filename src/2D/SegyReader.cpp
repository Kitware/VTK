#include "SegyReader.h"
#include <assert.h>

#include <vtkPoints.h>
#include <vtkCellArray.h>
#include <vtkQuad.h>
#include <vtkSmartPointer.h>
#include <vtkPolygon.h>
#include <vtkPolyData.h>
#include <vtkSmartPointer.h>
#include <vtkStructuredPointsReader.h>
#include <vtkVolumeTextureMapper3D.h>
#include <vtkColorTransferFunction.h>
#include <vtkPiecewiseFunction.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkVolumeProperty.h>
#include <vtkAxesActor.h>
#include <vtkImageShiftScale.h>
#include <vtkVolumeRayCastCompositeFunction.h>
#include <vtkVolumeRayCastMapper.h>
#include <vtkPolyDataMapper.h>
#include <vtkFloatArray.h>
#include <vtkPointData.h>
#include <vtkArrayData.h>
#include <vtkArray.h>
#include <vtkJPEGReader.h>

SegyReader::SegyReader() {
    isBigEndian = checkIfBigEndian();
}

bool SegyReader::LoadFromFile(string path) {
    ifstream in(path, ifstream::binary);
    if (!in) {
        cout << "File not found:" << path << endl;
        return false;
    }

    printBinaryHeader(in);

    cout << "here" << endl;

    readTextualHeader(in);
    readBinaryHeader(in);

    scanFile(in);

    int traceStartPos = 3600;// traces start after 3200 + 400 file header
    while (true) {
        if (!readTrace(traceStartPos, in, formatCode))
            break;
    }

    in.close();
    return true;
}


bool SegyReader::readTextualHeader(ifstream &in) {
    // TODO: these are only for waha8.sgy, should read from texual header
    crossLineNumberStep = 1;
    sampleCount = 1001;

    traceHeaderBytesPos.InlineNumber = 8;
    traceHeaderBytesPos.CrosslineNumber = 20;
}

bool SegyReader::readBinaryHeader(ifstream &in) {
    formatCode = getFormatCode(in);
}

bool SegyReader::checkIfBigEndian() {
    ushort a = 0x1234;
    if (*((unsigned char *) &a) == 0x12)
        return true;
    return false;
}

void SegyReader::scanFile(ifstream &in) {
    int startPos = 3600;
    minCrossLineNumber = INT_MAX;
    maxCrossLineNumber = INT_MIN;

    while (true) {

        if (startPos + 240 >= fileSize)
            break;

        in.seekg(startPos, in.beg);

        int crosslineNum = readLongInteger(startPos + traceHeaderBytesPos.CrosslineNumber, in);

        if (crosslineNum == 0)
            break;
        cout << "crossline number " << crosslineNum << endl;
        minCrossLineNumber = minCrossLineNumber < crosslineNum ? minCrossLineNumber : crosslineNum;
        maxCrossLineNumber = maxCrossLineNumber > crosslineNum ? maxCrossLineNumber : crosslineNum;

        int numSamples = readShortInteger(startPos + traceHeaderBytesPos.NumberSamples, in);
        startPos += 240 + getTraceSize(numSamples, formatCode);
    }

    crosslineNumberCount = (maxCrossLineNumber - minCrossLineNumber) / crossLineNumberStep + 1;

    cout << "Crossline Number Count " << crosslineNumberCount << endl;

    long dataSize = (long) crosslineNumberCount * sampleCount;
    data.resize(dataSize);
    for (long i = 0; i < dataSize; i++)
        data[i] = 0;

    traceCount = crosslineNumberCount;
    xCoordinates.resize(traceCount);
    for (long i = 0; i < traceCount; i++)
        xCoordinates[i] = 0;

    yCoordinates.resize(traceCount);
    for (long i = 0; i < traceCount; i++)
        yCoordinates[i] = 0;
}

int SegyReader::getFileSize(ifstream &in) {
    in.seekg(0, in.end);
    return in.tellg();
}

char SegyReader::readChar(ifstream &in) {
    char buffer;
    in.read(&buffer, sizeof(buffer));
    return buffer;
}

void SegyReader::printBinaryHeader(ifstream &in) {

    cout << "here" << endl;
    fileSize = getFileSize(in);
    cout << "file size:" << fileSize << endl;


    int jobID = readLongInteger(binaryHeaderBytesPos.JobID, in);
    cout << "Job identification number : " << jobID << endl;

    int lineNumber = readLongInteger(binaryHeaderBytesPos.LineNumber, in);
    cout << "Line number : " << lineNumber << endl;

    int reelNumber = readLongInteger(binaryHeaderBytesPos.ReelNumber, in);
    cout << "Reel number : " << reelNumber << endl;

    int numTracesPerEnsemble = readShortInteger(binaryHeaderBytesPos.NumberTracesPerEnsemble, in);
    cout << "Number of traces per ensemble: " << numTracesPerEnsemble << endl;

    int numAuxTracesPerEnsemble = readShortInteger(binaryHeaderBytesPos.NumberAuxTracesPerEnsemble, in);
    cout << "Number of auxiliary traces per ensemble : " << numAuxTracesPerEnsemble << endl;

    int sampleInterval = readShortInteger(binaryHeaderBytesPos.SampleInterval, in);
    cout << "Sample interval : " << sampleInterval << endl;

    int sampleIntervalOriginal = readShortInteger(binaryHeaderBytesPos.SampleIntervalOriginal, in);
    cout << "Sample interval original : " << sampleIntervalOriginal << endl;

    int numSamplesPerTrace = readShortInteger(binaryHeaderBytesPos.NumSamplesPerTrace, in);
    cout << "Number of samples per trace : " << numSamplesPerTrace << endl;

    int numSamplesOriginal = readShortInteger(binaryHeaderBytesPos.NumSamplesPerTraceOriginal, in);
    cout << "Number of samples per trace original : " << numSamplesOriginal << endl;

    int formatCode = readShortInteger(binaryHeaderBytesPos.FormatCode, in);
    cout << "format code : " << formatCode << endl;

    int numberExtendedHeaders = readShortInteger(binaryHeaderBytesPos.NumberExtendedHeaders, in);
    cout << "Number of extended headers : " << numberExtendedHeaders << endl;

    int ensembleType = readShortInteger(binaryHeaderBytesPos.EnsembleType, in);
    cout << "Ensemble type: " << ensembleType << endl;

    int version = readShortInteger(binaryHeaderBytesPos.Version, in);
    cout << "Version : " << version << endl;

    int fixedLengthFlag = readShortInteger(binaryHeaderBytesPos.FixedLengthFlag, in);
    cout << "Fixed length flag : " << fixedLengthFlag << endl;
}

#define PIXEL_TYPE float

void SegyReader::ExportData(vtkImageData *imageData) {
    cout << "Exporing data ... " << endl;

    float minValue, maxValue;
    minValue = maxValue = data[0];
    for (float element : data) {
        minValue = minValue < element ? minValue : element;
        maxValue = maxValue > element ? maxValue : element;
    }
    float bucketSize = (maxValue - minValue) / 256;

    vector<PIXEL_TYPE> pixels;
    for (float element : data) {
        PIXEL_TYPE pixel = (element - minValue) / bucketSize;
        pixels.push_back(pixel);
    }

    cout << "here" << endl;
    imageData->SetDimensions(sampleCount, crosslineNumberCount, 1);

    int type = VTK_FLOAT; //VTK_UNSIGNED_CHAR;
    imageData->SetScalarType(type, imageData->GetInformation());
    imageData->SetNumberOfScalarComponents(1, imageData->GetInformation());
    imageData->AllocateScalars(type, 1);
    PIXEL_TYPE *ptr = (PIXEL_TYPE *) imageData->GetScalarPointer();
    for (int k = 0; k < sampleCount; k++)
        for (int i = 0; i < crosslineNumberCount; i++) {
            //*(ptr + k * crosslineNumberCount + i)=
            //        pixels[i * sampleCount + k];

            *(ptr + i * sampleCount + k) =
                    pixels[i * sampleCount + k];
        }

    cout << "Export data finished ..." << endl;
}

void SegyReader::render2d(vtkActor *actor) {


    vtkColorTransferFunction *colorTransferFunction= vtkColorTransferFunction::New();
    colorTransferFunction->AddRGBPoint(0.000,  1.00, 0.00, 0.00);
    //colorTransferFunction->AddRGBPoint(60.000,  1.00, 1.00, 1.00);
    //colorTransferFunction->AddRGBPoint(128.00,  1.00, 1.0, 1.0);
    //makecolorTransferFunction->AddRGBPoint(200.000,  1.00, 1.00, 1.00);
    colorTransferFunction->AddRGBPoint(255.0,  0.00, 0.00, 1.00);


    vtkSmartPointer<vtkPoints> points =
            vtkSmartPointer<vtkPoints>::New();

    vtkSmartPointer<vtkFloatArray> textureCoordinates =
            vtkSmartPointer<vtkFloatArray>::New();
    textureCoordinates->SetNumberOfComponents(2);
    textureCoordinates->SetName("TextureCoordinates");

    for (int k = 0; k < sampleCount; k++) {
        for (int i = 0; i < crosslineNumberCount; i++) {
            float x = xCoordinates[i] / 100000.0;
            float y = yCoordinates[i] / 100000.0;
            float z = k * 100.0 / sampleCount;

            points->InsertNextPoint(x, y, z);

            cout << "x:" << x << ", y:" << y << ",z:" << z << endl;
            textureCoordinates->InsertNextTuple2(k * 1.0 / sampleCount, i * 1.0 / crosslineNumberCount);
        }
    }

// Create a cell array to store the quad in
    vtkSmartPointer<vtkCellArray> quads =
            vtkSmartPointer<vtkCellArray>::New();

    for (int k = 1; k < sampleCount; k++) {
        for (int i = 1; i < crosslineNumberCount; i++) {
            vtkSmartPointer<vtkPolygon> polygon =
                    vtkSmartPointer<vtkPolygon>::New();
            polygon->GetPointIds()->SetNumberOfIds(4); //make a quad

            int id1 = k * crosslineNumberCount + i;
            int id2 = (k - 1) * crosslineNumberCount + i;
            int id3 = (k - 1) * crosslineNumberCount + i - 1;
            int id4 = k * crosslineNumberCount + i - 1;
            polygon->GetPointIds()->SetId(0, id1);
            polygon->GetPointIds()->SetId(1, id2);
            polygon->GetPointIds()->SetId(2, id3);
            polygon->GetPointIds()->SetId(3, id4);
            quads->InsertNextCell(polygon);
        }
    }

    vtkSmartPointer<vtkTexture> texture =
            vtkSmartPointer<vtkTexture>::New();

    vtkSmartPointer<vtkImageData> imageData = vtkSmartPointer<vtkImageData>::New();
    ExportData(imageData);
    texture->SetInputDataObject(imageData);



// Create a polydata to store everything in
    vtkSmartPointer<vtkPolyData> polydata =
            vtkSmartPointer<vtkPolyData>::New();

// Add the points and quads to the dataset
    polydata->SetPoints(points);
    polydata->SetPolys(quads);
    polydata->GetPointData()->SetTCoords(textureCoordinates);

// Setup actor and mapper
    vtkSmartPointer<vtkPolyDataMapper> mapper =
            vtkSmartPointer<vtkPolyDataMapper>::New();

#if VTK_MAJOR_VERSION <= 5
mapper->SetInput(polydata);
#else
    mapper->SetInputData(polydata);
#endif
    mapper->SetLookupTable(colorTransferFunction);
    actor->SetMapper(mapper);
    actor->SetTexture(texture);
    return;

// Setup render window, renderer, and interactor
    vtkSmartPointer<vtkRenderer> renderer =
            vtkSmartPointer<vtkRenderer>::New();
    vtkSmartPointer<vtkRenderWindow> renderWindow =
            vtkSmartPointer<vtkRenderWindow>::New();
    renderWindow->AddRenderer(renderer);
    vtkSmartPointer<vtkRenderWindowInteractor> renderWindowInteractor =
            vtkSmartPointer<vtkRenderWindowInteractor>::New();
    renderWindowInteractor->SetRenderWindow(renderWindow);
    renderer->AddActor(actor);
    renderWindow->Render();
    renderWindowInteractor->Start();
//    vtkSmartPointer< vtkJPEGReader > reader =
//            vtkSmartPointer< vtkJPEGReader >::New();
//    reader->SetFileName("texture.jpg");
//
//    float minValue, maxValue;
//    minValue = maxValue = data[0];
//    for(float element : data)
//    {
//        minValue = minValue < element ? minValue : element;
//        maxValue = maxValue > element ? maxValue : element;
//    }
//    float bucketSize = (maxValue - minValue) / 256;
//
//
//    vector<unsigned char> pixels;
//    for(float element : data)
//    {
//        unsigned char pixel = (element - minValue) / bucketSize;
//        pixels.push_back(pixel);
//    }
//
//    vtkSmartPointer<vtkPoints> points =
//            vtkSmartPointer<vtkPoints>::New();
//
//    vtkSmartPointer<vtkFloatArray> textureCoordinates =
//            vtkSmartPointer<vtkFloatArray>::New();
//    textureCoordinates->SetNumberOfComponents(2);
//    textureCoordinates->SetName("TextureCoordinates");
//
//    normalizeCoordinates(xCoordinates);
//    normalizeCoordinates(yCoordinates);
//
//    for(int k=0; k<sampleCount; k++)
//    {
//        for(int i=0; i<crosslineNumberCount; i++)
//        {
//            float x = xCoordinates[k];
//            float y = yCoordinates[k];
//            float z = k * 1.0 / sampleCount;
//
//            points->InsertNextPoint(x, y, z);
//
//            //textureCoordinates->InsertNextTuple3(x, y, z);
//        }
//    }
//
//    //float scale = 1000;
//    //textureCoordinates->InsertNextTuple2(0, 0);
//    //textureCoordinates->InsertNextTuple2(1.0 * scale,0);
//    //textureCoordinates->InsertNextTuple2(1.0, 1.0 * scale);
//    //textureCoordinates->InsertNextTuple2(0, 1.0 * scale);
//
//    vtkSmartPointer<vtkCellArray> polygons =
//            vtkSmartPointer<vtkCellArray>::New();
//
//    for(int k=1; k<sampleCount; k++)
//    {
//        for(int i=1; i<crosslineNumberCount; i++)
//        {
//            vtkSmartPointer<vtkPolygon> polygon =
//                    vtkSmartPointer<vtkPolygon>::New();
//            polygon->GetPointIds()->SetNumberOfIds(4); //make a quad
//
//            int id1 = k * crosslineNumberCount + i;
//            int id2 = (k-1) * crosslineNumberCount + i;
//            int id3 = (k-1) * crosslineNumberCount + i - 1;
//            int id4 = k * crosslineNumberCount + i - 1;
//            polygon->GetPointIds()->SetId(0, id1);
//            polygon->GetPointIds()->SetId(1, id2);
//            polygon->GetPointIds()->SetId(2, id3);
//            polygon->GetPointIds()->SetId(3, id4);
//            polygons->InsertNextCell(polygon);
//        }
//    }
//
//    vtkSmartPointer<vtkPolyData> quad =
//            vtkSmartPointer<vtkPolyData>::New();
//    quad->SetPoints(points);
//    quad->SetPolys(polygons);
//    //quad->GetPointData()->SetTCoords(textureCoordinates);
//
//    //vtkSmartPointer<vtkTexture> texture =
//    //        vtkSmartPointer<vtkTexture>::New();
//
//
//    //vtkSmartPointer<vtkImageData> textureImage = vtkSmartPointer<vtkImageData>::New();
//    //textureImage->SetDimensions(sampleCount, crosslineNumberCount, traceNumberCount);
//
//
//
//
//    //texture->SetInputDataObject(textureImage);
//    //texture->SetInputConnection(reader->GetOutputPort());
//
//    vtkSmartPointer<vtkPolyDataMapper> mapper =
//            vtkSmartPointer<vtkPolyDataMapper>::New();
//    mapper->SetInputData(quad);
//
//    vtkSmartPointer<vtkActor> texturedQuad =
//            vtkSmartPointer<vtkActor>::New();
//    texturedQuad->SetMapper(mapper);
//    //texturedQuad->SetTexture(texture);
//
//    vtkSmartPointer<vtkRenderer> renderer =
//            vtkSmartPointer<vtkRenderer>::New();
//    renderer->AddActor(texturedQuad);
//    renderer->SetBackground(0,0,0); // Background color white
//    renderer->ResetCamera();
//
//    vtkSmartPointer<vtkRenderWindow> renderWindow =
//            vtkSmartPointer<vtkRenderWindow>::New();
//    renderWindow->AddRenderer(renderer);
//
//    vtkSmartPointer<vtkRenderWindowInteractor> renderWindowInteractor =
//            vtkSmartPointer<vtkRenderWindowInteractor>::New();
//    renderWindowInteractor->SetRenderWindow(renderWindow);
//
//    renderWindow->Render();
//    renderWindowInteractor->Start();

}

int SegyReader::readShortInteger(int pos, ifstream &in) {
    in.seekg(pos, in.beg);
    char buffer[2];
    in.read(buffer, sizeof(buffer));

    if (!isBigEndian) {
        swap(buffer, buffer + 1);
    }

    short num;
    memcpy(&num, buffer, 2);
    return num;
}

int SegyReader::getFormatCode(ifstream &in) {
    return readShortInteger(3224, in);
}

int SegyReader::readLongInteger(int pos, ifstream &in) {
    in.seekg(pos, in.beg);
    char buffer[4];
    in.read(buffer, sizeof(buffer));

    if (!isBigEndian) {
        swap(buffer, buffer + 3);
        swap(buffer + 1, buffer + 2);
    }

    int num;
    memcpy(&num, buffer, 4);
    return num;
}

void SegyReader::swap(char *a, char *b) {
    char temp = *a;
    *a = *b;
    *b = temp;
}

float SegyReader::readFloat(ifstream &in) {
    char buffer[4];
    in.read(buffer, sizeof(buffer));

    if (!isBigEndian) {
        swap(buffer, buffer + 3);
        swap(buffer + 1, buffer + 2);
    }

    float num;
    memcpy(&num, buffer, 4);
    return num;
}

void SegyReader::printTraceHeader(ifstream &in, int startPos) {

    cout << "Position:" << startPos << endl;

    int traceSequenceNumberInLine = readLongInteger(startPos + traceHeaderBytesPos.TraceNumber, in);
    cout << "Trace sequence number in line : " << traceSequenceNumberInLine << endl;

// Get number_of_samples from trace header position 115-116
    int numSamples = readShortInteger(startPos + traceHeaderBytesPos.NumberSamples, in);
    cout << "number of samples: " << numSamples << endl;

// Get inline number from trace header position 189-192
    int inlineNum = readLongInteger(startPos + traceHeaderBytesPos.InlineNumber, in);
    cout << "in-line number : " << inlineNum << endl;

    int crosslineNum = readLongInteger(startPos + traceHeaderBytesPos.CrosslineNumber, in);
    cout << "cross-line number : " << crosslineNum << endl;

    int xCoordinate = readLongInteger(startPos + traceHeaderBytesPos.XCoordinate, in);
    cout << "X coordinate for ensemble position of the trace : " << xCoordinate << endl;

    int yCoordinate = readLongInteger(startPos + traceHeaderBytesPos.YCoordinate, in);
    cout << "Y coordinate for ensemble position of the trace : " << yCoordinate << endl;
}

bool SegyReader::readTrace(int &startPos, ifstream &in, int formatCode) {
    if (startPos + 240 >= fileSize)
        return false;

    printTraceHeader(in, startPos);
    int crosslineNum = readLongInteger(startPos + traceHeaderBytesPos.CrosslineNumber, in);
    int numSamples = readShortInteger(startPos + traceHeaderBytesPos.NumberSamples, in);
    int xCoordinate = readLongInteger(startPos + traceHeaderBytesPos.XCoordinate, in);
    int yCoordinate = readLongInteger(startPos + traceHeaderBytesPos.YCoordinate, in);

    crosslineNum = (crosslineNum - minCrossLineNumber) / crossLineNumberStep;

    in.seekg(startPos + 240, in.beg);
    for (int i = 0; i < numSamples; i++) {
        float value = readFloat(in); // TODO: readChar or readFloat according to format code
        data[crosslineNum * sampleCount + i]
                = value;
    }

    xCoordinates[crosslineNum] = xCoordinate;
    yCoordinates[crosslineNum] = yCoordinate;

//cout << "x coordinate at " << inlineNumber * crosslineNumberCount + crosslineNum << " is " << xCoordinate << endl;
//cout << "trace count : " << traceCount << endl;

    startPos += 240 + getTraceSize(numSamples, formatCode);
    return true;
}

int SegyReader::getTraceSize(int numSamples, int formatCode) {
    if (formatCode == 1 || formatCode == 2 || formatCode == 4 || formatCode == 5) {
        return 4 * numSamples;
    }
    if (formatCode == 3) {
        return 2 * numSamples;
    }
    if (formatCode == 8) {
        return numSamples;
    }
    cout << "Unsupported data format code : " << formatCode << endl;
    assert(false);
}