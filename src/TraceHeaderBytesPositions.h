//
// Created by jia chen on 7/1/15.
//

#ifndef SEGYVISUALIZER_TRACEHEADERBYTESPOSITIONS_H
#define SEGYVISUALIZER_TRACEHEADERBYTESPOSITIONS_H
class TraceHeaderBytesPositions
{
public:
    int TraceNumber;
    int InlineNumber;
    int NumberSamples;
    int CrosslineNumber;
    int XCoordinate;
    int YCoordinate;
public:
    TraceHeaderBytesPositions()
    {
        initDefaultValues();
    }
private:
    void initDefaultValues()
    {
        TraceNumber = 0;
        InlineNumber = 188;
        NumberSamples = 114;
        CrosslineNumber = 192;
        XCoordinate = 180;
        YCoordinate = 184;
    }
};
#endif //SEGYVISUALIZER_TRACEHEADERBYTESPOSITIONS_H
