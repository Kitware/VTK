#include "SegyReader.h"
#include <assert.h>
SegyReader::SegyReader() {
    isBigEndian = checkIfBigEndian();
}

bool SegyReader::LoadFromFile(string path) {
    ifstream in(path, ifstream::binary);
    if(!in){
        cout << "File not found:" << path << endl;
        return false;
    }

    readTextualHeader(in);
    readBinaryHeader(in);

    scanFile(in);

    int traceStartPos = 3600;// traces start after 3200 + 400 file header
    for(int i=0; i<traceCount; i++)
    {
        readTrace(traceStartPos, in, formatCode);
    }

    in.close();
    return true;
}


bool SegyReader::readTextualHeader(ifstream &in) {
    // TODO: these are only for waha8.sgy, should read from texual header
    traceCount = 5760;
    crossLineNumberStep = 3;
    traceNumberStep = 3;
    sampleCount = 876;

    traceHeaderBytesPos.InlineNumber = 8;
    traceHeaderBytesPos.CrosslineNumber = 20;
}

bool SegyReader::readBinaryHeader(ifstream &in) {
    formatCode = getFormatCode(in);
}

bool SegyReader::checkIfBigEndian(){
    ushort a=0x1234;
    if (*((unsigned char *)&a)==0x12)
        return true;
    return false;
}

void SegyReader::scanFile(ifstream &in) {
    int startPos = 3600;
    minTraceNumber = INT_MAX;
    maxTraceNumber = INT_MIN;
    minCrossLineNumber = INT_MAX;
    maxCrossLineNumber = INT_MIN;

    for(int i=0; i<traceCount; i++) {
        in.seekg(startPos, in.beg);

        int inlineNumber = readLongInteger(startPos + traceHeaderBytesPos.InlineNumber, in);
        minTraceNumber = minTraceNumber < inlineNumber ? minTraceNumber : inlineNumber;
        maxTraceNumber = maxTraceNumber > inlineNumber ? maxTraceNumber : inlineNumber;

        int crosslineNum = readLongInteger(startPos + traceHeaderBytesPos.CrosslineNumber, in);
        minCrossLineNumber = minCrossLineNumber < crosslineNum ? minCrossLineNumber : crosslineNum;
        maxCrossLineNumber = maxCrossLineNumber > crosslineNum ? maxCrossLineNumber : crosslineNum;

        int numSamples = readShortInteger(startPos + traceHeaderBytesPos.NumberSamples, in);
        startPos += 240 + getTraceSize(numSamples, formatCode);

        cout << inlineNumber << ", " << crosslineNum << ", " << numSamples << endl;
    }

    traceNumberCount = (maxTraceNumber - minTraceNumber) / traceNumberStep + 1;
    crosslineNumberCount = (maxCrossLineNumber - minCrossLineNumber) / crossLineNumberStep + 1;

    long dataSize = (long)traceNumberCount * crosslineNumberCount * sampleCount;
    data.resize(dataSize);
    for(long i=0; i < dataSize; i++)
        data[i] = 0;
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

void SegyReader::printBinaryHeader(ifstream &in)
{
    cout << "file size:" << getFileSize(in) << endl;

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

void SegyReader::ExportData(vtkImageData* imageData)
{
    float minValue, maxValue;
    minValue = maxValue = data[0];
    for(float element : data)
    {
        minValue = minValue < element ? minValue : element;
        maxValue = maxValue > element ? maxValue : element;
    }
    float bucketSize = (maxValue - minValue) / 256;

    vector<unsigned char> pixels;
    for(float element : data)
    {
        unsigned char pixel = (element - minValue) / bucketSize;
        pixels.push_back(pixel);
    }
    imageData->SetDimensions(traceNumberCount, crosslineNumberCount, sampleCount);

    int type = VTK_UNSIGNED_CHAR;
    imageData->SetScalarType(type, imageData->GetInformation());
    imageData->SetNumberOfScalarComponents(1, imageData->GetInformation());
    imageData->AllocateScalars(type, 1);
    unsigned char *ptr=(unsigned char *)imageData->GetScalarPointer();
    for(int k=0; k<sampleCount; k++)
        for(int i=0;i<crosslineNumberCount;i++)
            for(int j=0;j<traceNumberCount;j++)
            {
                *(ptr + k * crosslineNumberCount * traceNumberCount + i * traceNumberCount + j)=
                        pixels[j*crosslineNumberCount*sampleCount + i * sampleCount + k];
            }
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

    if(!isBigEndian) {
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

    if(!isBigEndian) {
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
    cout << "number of samples: "<< numSamples << endl;

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
    //printTraceHeader(in, startPos);
    int inlineNumber = readLongInteger(startPos + traceHeaderBytesPos.InlineNumber, in) ;
    int crosslineNum = readLongInteger(startPos + traceHeaderBytesPos.CrosslineNumber, in);
    int numSamples = readShortInteger(startPos + traceHeaderBytesPos.NumberSamples, in);

    inlineNumber = ( inlineNumber - minTraceNumber) / traceNumberStep;
    crosslineNum = ( crosslineNum - minCrossLineNumber ) / crossLineNumberStep;

    in.seekg(startPos + 240, in.beg);
    for(int i=0; i<numSamples; i++)
    {
        unsigned char value = readChar(in); // TODO: readChar or readFloat according to format code
        data[ inlineNumber * crosslineNumberCount * sampleCount + crosslineNum * sampleCount + i ]
                = value;
    }
    startPos += 240 + getTraceSize(numSamples, formatCode);
}

int SegyReader::getTraceSize(int numSamples, int formatCode) {
    if(formatCode == 1 || formatCode == 2 || formatCode == 4 || formatCode == 5)
    {
        return 4 * numSamples;
    }
    if(formatCode == 3)
    {
        return 2 * numSamples;
    }
    if(formatCode == 8)
    {
        return numSamples;
    }
    cout << "Unsupported data format code : " << formatCode << endl;
    assert(false);
}