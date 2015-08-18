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
        InlineNumber = 8;
        NumberSamples = 114;
        CrosslineNumber = 20;
        XCoordinate = 72;
        YCoordinate = 76;
    }
};
#endif //SEGYVISUALIZER_TRACEHEADERBYTESPOSITIONS_H
