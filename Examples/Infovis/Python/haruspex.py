from vtk import *
import sys
import getopt

# Usage function
def Usage():
    print "Usage:"
    print "   -d <input data file> | -h <haruspex> | [-s <output model file> ] | [-a <output data file> ] | [-v]"
    sys.exit(1)
# End Usage function

# ReadInData function
# Read input CSV data as input port
def ReadInData( inDataName, verbosity ):
    if verbosity > 0:
        print "#  Reading input data:"

    inData = vtkDelimitedTextReader()
    inData.SetFieldDelimiterCharacters(",")
    inData.SetHaveHeaders(True)
    inData.SetDetectNumericColumns(True)
    inData.SetFileName(inDataName)
    inData.Update()

    if verbosity > 0:
        print "# Table loaded from CSV file:"
        T = inData.GetOutput()
        print "  Number of columns:", T.GetNumberOfColumns()
        print "  Number of rows:", T.GetNumberOfRows()
        print
    
    return inData
# End ReadInData function

# WriteOutData function
# Write haruspex output data
def WriteOutData( haruspex, outDataName, verbosity ):
    if verbosity > 0:
        print "#  Saving output (annotated) data:"

    outData = vtkDelimitedTextWriter()
    outData.SetFieldDelimiter(",")
    outData.SetFileName( outDataName )
    outData.SetInputConnection( haruspex.GetOutputPort(0) )
    outData.Update()

    if verbosity > 0:
        print "   Wrote", outDataName
        print
# End WriteOutData function

# WriteOutModel function
# Write haruspex output model
def WriteOutModel( haruspex, outModelName, verbosity ):
    if verbosity > 0:
        print "#  Saving output model (statistics):"
        
    outModel = vtkDelimitedTextWriter()
    outModel.SetFieldDelimiter(",")
    outModel.SetFileName(outModelName)

    if outModelType == "vtkTable":
        outModel.SetInputConnection(haruspex.GetOutputPort(1))
    elif outModelType == "vtkMultiBlockDataSet":
        print "ERROR: Unsupported case for now"
        sys.exit(1)

    outModel.Update()

    if verbosity > 0:
        print "   Wrote", outModelName
        print
# End WriteOutModel function

# Parse command line
verbosity = 1
print "# Parsing command line:"
opts,args = getopt.getopt(sys.argv[1:], 'vd:h:a:s:v:?')
inDataName = None
haruspexName = None
outDataName = "outputData.csv"
outModelName = "outputModel.csv"

for o,a in opts:
    if o == "-d":
        inDataName = a
    elif o == "-h":
        haruspexName = a
    elif o == "-a":
        outDataName = a
    elif o == "-s":
        outModelName = a
    elif o == "-v":
        verbosity += 1
    else:
        Usage()

if not inDataName:
    print "ERROR: a data file name required!"
    sys.exit(1)

if not haruspexName:
    print "ERROR: a haruspex name is required!"
    sys.exit(1)

# Verify that haruspex name makes sense and if so instantiate accordingly
if haruspexName == "descriptive":
    haruspex = vtkDescriptiveStatistics()
    numVariables = 1
    outModelType = "vtkTable"
elif haruspexName == "order":
    haruspex = vtkOrderStatistics()
    numVariables = 1
    outModelType = "vtkTable"
elif haruspexName == "correlative":
    haruspex = vtkCorrelativeStatistics()
    numVariables = 2
    outModelType = "vtkTable"
elif haruspexName == "contingency":
    haruspex = vtkContingencyStatistics()
    numVariables = 2
    outModelType = "vtkMultiBlockDataSet"
elif haruspexName == "multicorrelative":
    haruspex = vtkMultiCorrelativeStatistics()
    numVariables = 3
    outModelType = "vtkMultiBlockDataSet"
elif haruspexName == "PCA":
    haruspex = vtkPCAStatistics()
    numVariables = 3
    outModelType = "vtkMultiBlockDataSet"
else:
    print "ERROR: Invalid haruspex name:", haruspexName
    sys.exit(1)

print "  Input data file:", inDataName
print "  Haruspex:", haruspexName, "( number of variables:", numVariables,")"
print

# Get input data port
inData = ReadInData( inDataName, verbosity )
T = inData.GetOutput()
if verbosity > 1:
    T.Dump( 10 )

# Prepare haruspex
print "# Calculating descriptive statistics:"
haruspex.AddInputConnection(inData.GetOutputPort())


# Generate list of columns of interest, depending on number of variables
if numVariables == 1:
    # Univariate case: one request for each columns
    for i in range(0,T.GetNumberOfColumns()):
        colName = T.GetColumnName(i)
        print "  Requesting column",colName
        haruspex.AddColumn(colName)
elif numVariables == 2:
    # Bivariate case: generate all possible pairs
    for i in range(0,T.GetNumberOfColumns()):
        colNameX = T.GetColumnName(i)
        for j in range(i+1,T.GetNumberOfColumns()):
            colNameY = T.GetColumnName(j)
            print "  Requesting column pair",colNameX,colNameY
            haruspex.AddColumnPair(colNameX,colNameY)
elif numVariables == 3:
    # Multivariate case: generate single request containing all columns
    for i in range(0,T.GetNumberOfColumns()):
        colName = T.GetColumnName(i)
        haruspex.SetColumnStatus( colName, 1 )
        print "  Adding column", colName, "to the request"
else:
    print "ERROR: Unsupported case: number of variables = ",numVariables
    sys.exit(1)
    
haruspex.RequestSelectedColumns()

# Calculate statistics with Learn, Derive, and Assess options turned on (Test is left out for now)
haruspex.SetLearnOption(True)
haruspex.SetDeriveOption(True)
haruspex.SetAssessOption(True)
haruspex.SetTestOption(False)
haruspex.Update()
print "  Completed calculations"
print

# Save output (annotated) data
WriteOutData( haruspex, outDataName, verbosity )

# Save output model (statistics)
WriteOutModel( haruspex, outModelName, verbosity )
