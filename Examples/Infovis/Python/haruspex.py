############################################################
from vtk import *
import sys
import getopt
############################################################

############################################################
# Parse command line
def ParseCommandLine():
    opts,args = getopt.getopt(sys.argv[1:], 'vd:e:a:s:v:h')

    # Default values
    verbosity = 0
    outModelName = "outputModel.csv"
    outDataName = "outputData.csv"
    
    # First verify that the helper has not been requested (supersedes everything else)
    # NB: handled first and separately so default values cannot be altered in helper message
    for o,a in opts:
        if o == "-h":
            Usage( outModelName, outDataName, verbosity )

    # Parse arguments and assign corresponding variables
    for o,a in opts:
        if o == "-d":
            inDataName = a
        elif o == "-e":
            haruspexName = a
        elif o == "-a":
            outDataName = a
        elif o == "-s":
            outModelName = a
        elif o == "-v":
            verbosity += 1

    if not inDataName:
        print "ERROR: a data file name required!"
        sys.exit(1)
        
    if not haruspexName:
        print "ERROR: a statistics engine name is required!"
        sys.exit(1)

    return [ inDataName, haruspexName, outModelName, outDataName, verbosity ]
############################################################

############################################################
# Turn haruspex name into vtkStatistics object and ancillary parameters
def InstantiateStatistics( haruspexName ):
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
        print "ERROR: Invalid statistics engine:", haruspexName
        sys.exit(1)

    return [ haruspex, numVariables, outModelType ]
############################################################

############################################################
# Usage function
def Usage( outModelName, outDataName, verbosity ):
    print "Usage:"
    print "\t -h               Help: print this message and exit"
    print "\t -d <filename>    CSV input data file"
    print "\t -e <haruspex>    Type of statistics engine"
    print "\t [-s <filename> ] CSV output model (statistics) file. Default:",outModelName
    print "\t [-a <filename> ] CSV output data (annotated) file. Default:",outDataName
    print "\t [-v]             Increase verbosity (0 = silent). Default:",verbosity
    sys.exit(1)
############################################################

############################################################
# Read input CSV data as input port
def ReadInData( inDataName, verbosity ):
    if verbosity > 0:
        print "# Reading input data:"

    inData = vtkDelimitedTextReader()
    inData.SetFieldDelimiterCharacters(",")
    inData.SetHaveHeaders(True)
    inData.SetDetectNumericColumns(True)
    inData.SetFileName(inDataName)
    inData.Update()

    if verbosity > 0:
        T = inData.GetOutput()
        print "  Number of columns:", T.GetNumberOfColumns()
        print "  Number of rows:", T.GetNumberOfRows()
        print
    
    return inData
############################################################

############################################################
# Write haruspex output data
def WriteOutData( haruspex, outDataName, verbosity ):
    if verbosity > 0:
        print "# Saving output (annotated) data:"

    outData = vtkDelimitedTextWriter()
    outData.SetFieldDelimiter(",")
    outData.SetFileName( outDataName )
    outData.SetInputConnection( haruspex.GetOutputPort(0) )
    outData.Update()

    if verbosity > 0:
        print "  Wrote", outDataName
        print
############################################################

############################################################
# Write haruspex output model
def WriteOutModel( haruspex, outModelName, verbosity ):
    if verbosity > 0:
        print "# Saving output model (statistics):"
        
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
        print "  Wrote", outModelName
        print
############################################################

############################################################
# Calculate statistics
def CalculateStatistics( haruspex ):
    # Get the input connection
    haruspex.AddInputConnection(inData.GetOutputPort())

    # Generate list of columns of interest, depending on number of variables
    if numVariables == 1:
        # Univariate case: one request for each columns
        for i in range(0,T.GetNumberOfColumns()):
            colName = T.GetColumnName(i)
            if verbosity > 0:
                print "  Requesting column",colName
            haruspex.AddColumn(colName)
    elif numVariables == 2:
        # Bivariate case: generate all possible pairs
        for i in range(0,T.GetNumberOfColumns()):
            colNameX = T.GetColumnName(i)
            for j in range(i+1,T.GetNumberOfColumns()):
                colNameY = T.GetColumnName(j)
                if verbosity > 0:
                    print "  Requesting column pair",colNameX,colNameY
                haruspex.AddColumnPair(colNameX,colNameY)
    elif numVariables == 3:
        # Multivariate case: generate single request containing all columns
        for i in range(0,T.GetNumberOfColumns()):
            colName = T.GetColumnName(i)
            haruspex.SetColumnStatus( colName, 1 )
            if verbosity > 0:
                print "  Adding column", colName, "to the request"
    else:
        print "ERROR: Unsupported case: number of variables = ",numVariables
        sys.exit(1)

    # Complete column selection request
    haruspex.RequestSelectedColumns()
    
    # Calculate statistics with Learn, Derive, and Assess options turned on (Test is left out for now)
    haruspex.SetLearnOption(True)
    haruspex.SetDeriveOption(True)
    haruspex.SetAssessOption(True)
    haruspex.SetTestOption(False)
    haruspex.Update()
############################################################

# Parse command line
[ inDataName, haruspexName, outModelName, outDataName, verbosity ] = ParseCommandLine()
if verbosity > 0:
    print "# Parsed command line:"
    print "  Input data file:", inDataName
    print "  Statistics:", haruspexName
    print "  Output model file:", outModelName
    print "  Output data file:", outDataName
    print

# Verify that haruspex name makes sense and if so instantiate accordingly
[ haruspex, numVariables, outModelType ] = InstantiateStatistics( haruspexName )
if verbosity > 0:
    print "# Instantiated a", haruspex.GetClassName(), "object"
    print "  Number of variables:", numVariables
    print "  Type of output model file:", outModelType
    print

# Get input data port
inData = ReadInData( inDataName, verbosity )
T = inData.GetOutput()
if verbosity > 1:
    print "# Input data:"
    T.Dump( 10 )
    print

# Calculate statistics
if verbosity > 0:
    print "# Calculating statistics:"
CalculateStatistics( haruspex )
if verbosity > 0:
    print "  Done"
    print

# Save output (annotated) data
WriteOutData( haruspex, outDataName, verbosity )

# Save output model (statistics)
WriteOutModel( haruspex, outModelName, verbosity )
