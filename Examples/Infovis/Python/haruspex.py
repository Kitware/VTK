############################################################
from vtk import *
import sys
import getopt
############################################################

############################################################
# Global variable for convenience
verbosity = 0
############################################################

############################################################
# Usage function
def Usage( outModelPrefix, outDataName ):
    print "Usage:"
    print "\t -h               Help: print this message and exit"
    print "\t -d <filename>    CSV input data file"
    print "\t -e <haruspex>    Type of statistics engine. Available engines are:"
    print "\t                    descriptive"
    print "\t                    order"
    print "\t                    contingency"
    print "\t                    correlative"
    print "\t                    multicorrelative"
    print "\t                    pca"
    print "\t                    kmeans"
    print "\t [-s <filename> ] CSV output model (statistics) file prefix. Default:",outModelPrefix
    print "\t [-a <filename> ] CSV output data (annotated) file. Default:",outDataName
    print "\t [-v]             Increase verbosity (0 = silent). Default:",verbosity
    sys.exit( 1 )
############################################################

############################################################
# Parse command line
def ParseCommandLine():
    # Declare use of global variable
    global verbosity

    # Default values
    outModelPrefix = "outputModel"
    outDataName = "outputData.csv"
    
    # Try to hash command line with respect to allowable flags
    try:
        opts,args = getopt.getopt(sys.argv[1:], 'vd:e:a:s:v:h')
    except getopt.GetoptError:
        Usage( outModelPrefix, outDataName )
        sys.exit( 1 )

    # First verify that the helper has not been requested (supersedes everything else)
    # NB: handled first and separately so default values cannot be altered in helper message
    for o,a in opts:
        if o == "-h":
            Usage( outModelPrefix, outDataName )

    # Parse arguments and assign corresponding variables
    for o,a in opts:
        if o == "-d":
            inDataName = a
        elif o == "-e":
            haruspexName = a
        elif o == "-a":
            outDataName = a
        elif o == "-s":
            outModelPrefix = a
        elif o == "-v":
            verbosity += 1

    if not inDataName:
        print "ERROR: a data file name required!"
        sys.exit( 1 )
        
    if not haruspexName:
        print "ERROR: a statistics engine name is required!"
        sys.exit( 1 )

    if verbosity > 0:
        print "# Parsed command line:"
        print "  Input data file:", inDataName
        print "  Statistics:", haruspexName
        print "  Output model file prefix:", outModelPrefix
        print "  Output data file:", outDataName
        print

    return [ inDataName, haruspexName, outModelPrefix, outDataName ]
############################################################

############################################################
# Turn haruspex name into vtkStatistics object and ancillary parameters
def InstantiateStatistics( haruspexName ):
    # Declare use of global variable
    global verbosity

    if haruspexName == "descriptive":
        haruspex = vtkDescriptiveStatistics()

    elif haruspexName == "order":
        haruspex = vtkOrderStatistics()

    elif haruspexName == "contingency":
        haruspex = vtkContingencyStatistics()

    elif haruspexName == "correlative":
        haruspex = vtkCorrelativeStatistics()

    elif haruspexName == "multicorrelative":
        haruspex = vtkMultiCorrelativeStatistics()

    elif haruspexName == "pca":
        haruspex = vtkPCAStatistics()

    elif haruspexName == "kmeans":
        haruspex = vtkKMeansStatistics()

    else:
        print "ERROR: Invalid statistics engine:", haruspexName
        sys.exit( 1 )

    if verbosity > 0:
        print "# Instantiated a", haruspex.GetClassName(), "object"
        print

    return haruspex
############################################################

############################################################
# Read input CSV data as input port
def ReadInData( inDataName ):
    # Declare use of global variable
    global verbosity

    if verbosity > 0:
        print "# Reading input data:"

    # Set CSV reader parameters
    inData = vtkDelimitedTextReader()
    inData.SetFieldDelimiterCharacters(",")
    inData.SetHaveHeaders(True)
    inData.SetDetectNumericColumns(True)
    inData.SetFileName(inDataName)
    inData.Update()

    if verbosity > 0:
        table = inData.GetOutput()
        print "  Number of columns:", table.GetNumberOfColumns()
        print "  Number of rows:", table.GetNumberOfRows()
        print
        if verbosity > 1:
            print "# Input data:"
            inData.GetOutput().Dump( 10 )
            print
    
    return inData
############################################################

############################################################
# Write haruspex output data
def WriteOutData( haruspex, outDataName ):
    # Declare use of global variable
    global verbosity

    if verbosity > 0:
        print "# Saving output (annotated) data:"

    # Set CSV writer parameters
    outDataWriter = vtkDelimitedTextWriter()
    outDataWriter.SetFieldDelimiter(",")
    outDataWriter.SetFileName( outDataName )
    outDataWriter.SetInputConnection( haruspex.GetOutputPort( 0 ) )
    outDataWriter.Update()

    if verbosity > 0:
        print "  Wrote", outDataName
        print
############################################################

############################################################
# Write haruspex output model
def WriteOutModel( haruspex, outModelPrefix ):
    # Declare use of global variable
    global verbosity

    if verbosity > 0:
        print "# Saving output model (statistics):"
        
    # Set CSV writer parameters
    outModelWriter = vtkDelimitedTextWriter()
    outModelWriter.SetFieldDelimiter(",")

    # Get output model type to select appropriate write scheme
    outModelType = haruspex.GetOutputDataObject( 1 ).GetClassName()
    if verbosity > 0:
        print "  Output model is a", outModelType

    # Select write scheme depending on output model type
    if outModelType == "vtkTable":
        # Straightforward CSV file dump of a vtkTable
        outModelName = outModelPrefix + ".csv"
        outModelWriter.SetFileName( outModelName )
        outModelWriter.SetInputConnection( haruspex.GetOutputPort( 1 ) )
        outModelWriter.Update()

        if verbosity > 0:
            print "  Wrote", outModelPrefix
            if verbosity > 1:
                haruspex.GetOutput( 1 ).Dump( 10 )

    elif outModelType == "vtkMultiBlockDataSet":
        # Must iterate over all blocks of the vtkMultiBlockDataSet
        outModel = haruspex.GetOutputDataObject( 1 )
        n = outModel.GetNumberOfBlocks()
        for i in range( 0, n ):
            # Straightforward CSV file dump of a vtkTable
            outModelName = outModelPrefix + "-" + str( i )+ ".csv"
            outModelWriter.SetFileName( outModelName )
            table = outModel.GetBlock( i )
            outModelWriter.SetInput( table )
            outModelWriter.Update()
            
            if verbosity > 0:
                print "  Wrote", outModelName
                if verbosity > 1:
                    table.Dump( 10 )

    if verbosity > 0:
        print
############################################################

############################################################
# Calculate statistics
def CalculateStatistics( inData, haruspex ):
    # Declare use of global variable
    global verbosity

    if verbosity > 0:
        print "# Calculating statistics:"

    # Output port of data reader becomes input connection of haruspex
    haruspex.AddInputConnection(inData.GetOutputPort() )

    # Get the output table of the data reader
    table = inData.GetOutput()

    # Generate list of columns of interest, depending on number of variables
    if haruspex.IsA( "vtkUnivariateStatisticsAlgorithm" ):
        # Univariate case: one request for each columns
        for i in range( 0, table.GetNumberOfColumns() ):
            colName = table.GetColumnName( i )
            if verbosity > 0:
                print "  Requesting column",colName
            haruspex.AddColumn(colName)

    elif haruspex.IsA( "vtkBivariateStatisticsAlgorithm" ):
        # Bivariate case: generate all possible pairs
        for i in range( 0, table.GetNumberOfColumns() ):
            colNameX = table.GetColumnName( i )
            for j in range( i+1,table.GetNumberOfColumns() ):
                colNameY = table.GetColumnName( j )
                if verbosity > 0:
                    print "  Requesting column pair",colNameX,colNameY
                haruspex.AddColumnPair(colNameX,colNameY)

    else:
        # Multivariate case: generate single request containing all columns
        for i in range( 0, table.GetNumberOfColumns() ):
            colName = table.GetColumnName( i )
            haruspex.SetColumnStatus( colName, 1 )
            if verbosity > 0:
                print "  Adding column", colName, "to the request"

    # Complete column selection request
    haruspex.RequestSelectedColumns()
    
    # Calculate statistics with Learn, Derive, and Assess options turned on (Test is left out for now)
    haruspex.SetLearnOption(True)
    haruspex.SetDeriveOption(True)
    haruspex.SetAssessOption(True)
    haruspex.SetTestOption(False)
    haruspex.Update()

    if verbosity > 0:
        print "  Done"
        print
############################################################

############################################################
# Main function
def main():
    # Parse command line
    [ inDataName, haruspexName, outModelPrefix, outDataName ] = ParseCommandLine()

    # Verify that haruspex name makes sense and if so instantiate accordingly
    haruspex = InstantiateStatistics( haruspexName )

    # Get input data port
    inData = ReadInData( inDataName )

    # Calculate statistics
    CalculateStatistics( inData, haruspex )

    # Save output (annotated) data
    WriteOutData( haruspex, outDataName )

    # Save output model (statistics)
    WriteOutModel( haruspex, outModelPrefix )
############################################################

############################################################
if __name__ == "__main__":
    main()
############################################################
