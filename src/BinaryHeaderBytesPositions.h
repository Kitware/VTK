#ifndef SEGYVISUALIZER_BINARYHEADERBYTESPOSITIONS_H
#define SEGYVISUALIZER_BINARYHEADERBYTESPOSITIONS_H
class BinaryHeaderBytesPositions
{
public:
    int JobID;
    int LineNumber;
    int ReelNumber;
    int NumberTracesPerEnsemble;
    int NumberAuxTracesPerEnsemble;
    int SampleInterval;
    int SampleIntervalOriginal;
    int NumSamplesPerTrace;
    int NumSamplesPerTraceOriginal;
    int FormatCode;
    int NumberExtendedHeaders;
    int EnsembleType;
    int Version;
    int FixedLengthFlag;
public:
    BinaryHeaderBytesPositions()
    {
        initDefaultValues();
    }
private:
    void initDefaultValues()
    {
        JobID = 3200;
        LineNumber = 3204;
        ReelNumber = 3208;
        NumberTracesPerEnsemble = 3212;
        NumberAuxTracesPerEnsemble = 3214;
        SampleInterval = 3216;
        SampleIntervalOriginal = 3218;
        NumSamplesPerTrace = 3220;
        NumSamplesPerTraceOriginal = 3222;
        FormatCode = 3224;
        NumberExtendedHeaders = 3502;
        EnsembleType = 3228;
        Version = 3500;
        FixedLengthFlag = 3502;
    }
};
#endif //SEGYVISUALIZER_BINARYHEADERBYTESPOSITIONS_H
