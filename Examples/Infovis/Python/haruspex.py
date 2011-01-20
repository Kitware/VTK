############################################################
# Copyright 2010 Sandia Corporation.
# Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
# the U.S. Government retains certain rights in this software.
############################################################
# Contact: Philippe Pebay, Sandia National Laboratories, pppebay@sandia.gov
############################################################

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
    print "\t [-h]             Help: print this message and exit"
    print "\t -d <filename>    name of CSV input data file"
    print "\t [-c <filename>]  name of CSV file specifying columns of interest. Default: all columns are of interest"
    print "\t -e <engine>      Type of statistics engine. Available engines are:"
    print "\t                    descriptive"
    print "\t                    order"
    print "\t                    contingency"
    print "\t                    correlative"
    print "\t                    multicorrelative"
    print "\t                    pca"
    print "\t                    kmeans"
    print "\t [-o <bitcode>]   Engine options bitcode. Default is 0. Available bits are:"
    print "\t                    1st bit: assess"
    print "\t                    2nd bit: test"
    print "\t [-m <prefix>]    prefix of CSV input model file(s). Default: calculate model from scratch"
    print "\t [-u]             update input model (if data are provided as well). NB: update happens before assessment"
    print "\t [-s <prefix>]    prefix of CSV output model (statistics) file(s)"
    print "\t [-a <filename>]  name of CSV output data (annotated) file"
    print "\t [-t <filename>]  name of CSV statistical test results file"
    print "\t [-v]             Increase verbosity (from no flag = silent to -vvv = print all tables)"
    sys.exit( 1 )
############################################################

############################################################
# Parse command line
def ParseCommandLine():
    # Declare use of global variable
    global verbosity

    # Default values
    options = 0
    inDataName = ""
    inModelPrefix = ""
    updateModel = False
    haruspexName = ""
    outModelPrefix = ""
    outDataName = ""
    outTestName = ""
    columnsListName =""
    
    # Try to hash command line with respect to allowable flags
    try:
        opts,args = getopt.getopt(sys.argv[1:], 'hd:e:o:m:us:a:t:c:v')
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
        elif o == "-o":
            options = a
        elif o == "-m":
            inModelPrefix = a
        elif o == "-u":
            updateModel = True
        elif o == "-s":
            outModelPrefix = a
        elif o == "-a":
            outDataName = a
        elif o == "-t":
            outTestName = a
        elif o == "-c":
            columnsListName = a
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
        if inModelPrefix != "":
            print "  Input model file prefix:", inModelPrefix
        else:
            print "  No input model"

        print "  Statistics:", haruspexName
        if columnsListName != "":
            print "  Columns of interest in file:", columnsListName
        else:
            print "  Columns of interest: all"

        print "  Output data file:", outDataName
        print "  Output model file prefix:", outModelPrefix

        print

    return [ inDataName, \
             inModelPrefix, \
             updateModel, \
             columnsListName, \
             haruspexName, \
             options, \
             outDataName, \
             outTestName, \
             outModelPrefix ]
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
# Read input CSV model table as input port
def ReadInModelTable( inModelPrefix, tabIndex ):
    # Declare use of global variable
    global verbosity

    if verbosity > 0:
        print "# Reading input model table", tabIndex

    # Set CSV reader parameters
    inTableReader = vtkDelimitedTextReader()
    inTableReader.SetFieldDelimiterCharacters(",")
    inTableReader.SetHaveHeaders( True )
    inTableReader.SetDetectNumericColumns( True )
    inTableReader.SetFileName( inModelPrefix + "-" + str( tabIndex ) + ".csv" )
    inTableReader.Update()

    if verbosity > 0:
        table = inTableReader.GetOutput()
        print "  Number of columns:", table.GetNumberOfColumns()
        print "  Number of rows:", table.GetNumberOfRows()
        if verbosity > 1:
            inTableReader.GetOutput().Dump( 16 )
        print

    return inTableReader
############################################################

############################################################
# Read input CSV data as input port
def ReadInData( inDataName ):
    # Declare use of global variable
    global verbosity

    if verbosity > 0:
        print "# Reading input data"

    # Set CSV reader parameters
    inDataReader = vtkDelimitedTextReader()
    inDataReader.SetFieldDelimiterCharacters(",")
    inDataReader.SetHaveHeaders( True )
    inDataReader.SetDetectNumericColumns( True )
    inDataReader.SetFileName( inDataName )
    inDataReader.Update()

    if verbosity > 0:
        table = inDataReader.GetOutput()
        print "  Number of columns:", table.GetNumberOfColumns()
        print "  Number of rows:", table.GetNumberOfRows()
        print
        if verbosity > 2:
            print "# Input data:"
            inDataReader.GetOutput().Dump( 16 )
            print
    
    return inDataReader
############################################################

############################################################
# Read list of columns of interest
def ReadColumnsList( columnsListName ):
    # Declare use of global variable
    global verbosity

    if verbosity > 0:
        print "# Reading list of columns of interest:"

    # Set CSV reader parameters
    columnsListReader = vtkDelimitedTextReader()
    columnsListReader.SetFieldDelimiterCharacters(",")
    columnsListReader.SetHaveHeaders( False )
    columnsListReader.SetDetectNumericColumns( True )
    columnsListReader.SetFileName( columnsListName )
    columnsListReader.Update()

    # Figure number of columns of interest
    table = columnsListReader.GetOutput()
    n = table.GetNumberOfColumns()
    if verbosity > 0:
        print "  Number of columns of interest:", n

    # Now construct list of colums of interest
    columnsList = []
    for i in range( 0, n ):
        columnsList.append( table.GetColumn( i ).GetValue( 0 ) )
    if verbosity > 1:
        print "  Columns of interest are:", columnsList

    if verbosity > 0:
        print

    return columnsList
############################################################

############################################################
# Write table from haruspex output port (i.e., for data or tests)
def WriteOutTable( haruspex, outPort, outFileName, outPortName, threshold ):
    # Declare use of global variable
    global verbosity

    if outFileName == "":
        if verbosity > 0:
            print "# No output table of", outPortName, "required"
            print
        return

    if verbosity > 0:
        print "# Saving output table of", outPortName

    # Set CSV writer parameters
    outTableWriter = vtkDelimitedTextWriter()
    outTableWriter.SetFieldDelimiter(",")
    outTableWriter.SetFileName( outFileName )
    outTableWriter.SetInputConnection( haruspex.GetOutputPort( outPort ) )
    outTableWriter.Update()

    if verbosity > 0:
        print "  Wrote", outPortName
        if verbosity > threshold:
            haruspex.GetOutput( outPort ).Dump( 16 )
        print
############################################################

############################################################
# Write haruspex output model
def WriteOutModel( haruspex, outModelPrefix ):
    # Declare use of global variable
    global verbosity

    if outModelPrefix == "":
        if verbosity > 0:
            print "# No output model (statistics) required"
            print
        return

    if verbosity > 0:
        print "# Saving output model (statistics):"
        
    # Set CSV writer parameters
    outModelWriter = vtkDelimitedTextWriter()
    outModelWriter.SetFieldDelimiter(",")

    # Verify that model is a vtkMultiBlockDataSet, error out otherwise
    outModelType = haruspex.GetOutputDataObject( 1 ).GetClassName()
    if outModelType != "vtkMultiBlockDataSet":
        print "ERROR: unsupported type of output model!"
        sys.exit( 1 )

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
                table.Dump( 16 )
                print
############################################################

############################################################
# Calculate statistics
def CalculateStatistics( inDataReader, inModelReader, updateModel, columnsList, haruspex, options ):
    # Declare use of global variable
    global verbosity

    if verbosity > 0:
        print "# Calculating statistics:"

    # Output port of data reader becomes input connection of haruspex
    haruspex.AddInputConnection( inDataReader.GetOutputPort() )

    # Get the output table of the data reader, which becomes the input data
    inData = inDataReader.GetOutput()

    # Figure number of columns of interest. If no list was provided, use them all
    if columnsList == []:
        columnsList = range( 0, inData.GetNumberOfColumns() )
    n = len( columnsList )
    
    # Generate list of columns of interest, depending on number of variables
    if haruspex.IsA( "vtkUnivariateStatisticsAlgorithm" ):
        # Univariate case: one request for each columns
        for i in range( 0, n ):
            colName = inData.GetColumnName( columnsList[i] )
            if verbosity > 0:
                print "  Requesting column", colName
            haruspex.AddColumn( colName )

    elif haruspex.IsA( "vtkBivariateStatisticsAlgorithm" ):
        # Bivariate case: generate all possible pairs
        for i in range( 0, n ):
            colNameX = inData.GetColumnName( columnsList[i] )
            for j in range( i+1, n ):
                colNameY = inData.GetColumnName( columnsList[j] )
                if verbosity > 0:
                    print "  Requesting column pair (", colNameX, ",", colNameY, ")"
                haruspex.AddColumnPair( colNameX, colNameY )

    else:
        # Multivariate case: generate single request containing all columns
        for i in range( 0, n ):
            colName = inData.GetColumnName( columnsList[i] )
            haruspex.SetColumnStatus( colName, 1 )
            if verbosity > 0:
                print "  Adding column", colName, "to the request"

    # Complete column selection request
    haruspex.RequestSelectedColumns()
    
    # Figure which options were requested
    if int( options ) % 2:
        assessOption = True
        if verbosity > 0:
            print "  Assess option is on"
    else:
        assessOption = False
        if verbosity > 0:
            print "  Assess option is off"
    options = int( options ) >> 1
    if int( options ) % 2:
        haruspex.SetTestOption( True )
        if verbosity > 0:
            print "  Test option is on"
    else:
        haruspex.SetTestOption( False )
        if verbosity > 0:
            print "  Test option is off"

    if verbosity > 0:
        print

    # If an input model was provided, then update it first, otherwise run in a single pass
    if inModelReader == None:
        # No model reader: then Learn, Derive, and possibly Assess in a single pass
        haruspex.SetLearnOption( True )
        haruspex.SetDeriveOption( True )
        haruspex.SetAssessOption( assessOption )
        haruspex.Update()
    else:
        # Model readers are available: decide how many tables will be fetched
        nPrimaryTables = haruspex.GetNumberOfPrimaryTables()

        # Then create vtkMultiBlockDataSet with correspondingly many blocks
        inModel = vtkMultiBlockDataSet()
        inModel.SetNumberOfBlocks( nPrimaryTables )

        # Now iterate over all readers to obtain tables
        for t in range( 0, nPrimaryTables ):
            inTableReader = inModelReader[t]
            inTable = inTableReader.GetOutput()

            # Handle special case of second table of order statistics
            if ( t > 0 and haruspex.GetClassName() == "vtkOrderStatistics" ):
                if verbosity > 0:
                    print "# Converting input order table to appropriate column types"

                # Create a programmable filter whose input is the order table
                convertOrderTab = vtkProgrammableFilter()
                convertOrderTab.SetInput( inTable )

                # Define table converter callback for programmable filter
                def ConvertOrderTableCallback():
                    readTable = convertOrderTab.GetInput()
                    convTable = convertOrderTab.GetOutput()

                    # Create columns with appropriate names and formats
                    kCol = vtkIdTypeArray()
                    kCol.SetName( "Key" )
                    convTable.AddColumn( kCol )
                    xCol = vtkStringArray()
                    xCol.SetName( "Value" )
                    convTable.AddColumn( xCol )
                    cCol = vtkIdTypeArray()
                    cCol.SetName( "Cardinality" )
                    convTable.AddColumn( cCol )

                    # Loop over all input rows and create output rows
                    nRow = readTable.GetNumberOfRows()
                    row = vtkVariantArray()
                    row.SetNumberOfValues( 3 )
                    for r in range( 0, nRow ):
                        # Retrieve primary statistics and convert to correct type
                        k = readTable.GetValueByName( r, "Key" ).ToInt()
                        row.SetValue( 0, k )
                        x = readTable.GetValueByName( r, "Value" ).ToString()
                        row.SetValue( 1, x )
                        c = readTable.GetValueByName( r, "Cardinality" ).ToInt()
                        row.SetValue( 2, c )

                        convTable.InsertNextRow( row )

                # Set callback and run programmable filer
                convertOrderTab.SetExecuteMethod( ConvertOrderTableCallback )
                convertOrderTab.Update()

                # Retrieve converted table from filter output
                inTable = convertOrderTab.GetOutput()
                if verbosity > 1:
                    inTable.Dump( 16 )

            # Handle special case of second table of contingency statistics
            if ( t > 0 and haruspex.GetClassName() == "vtkContingencyStatistics" ):
                if verbosity > 0:
                    print "# Converting input contingency table to appropriate column types"

                # Create a programmable filter whose input is the contingency table
                convertContingencyTab = vtkProgrammableFilter()
                convertContingencyTab.SetInput( inTable )

                # Define table converter callback for programmable filter
                def ConvertContingencyTableCallback():
                    readTable = convertContingencyTab.GetInput()
                    convTable = convertContingencyTab.GetOutput()

                    # Create columns with appropriate names and formats
                    kCol = vtkIdTypeArray()
                    kCol.SetName( "Key" )
                    convTable.AddColumn( kCol )
                    xCol = vtkStringArray()
                    xCol.SetName( "x" )
                    convTable.AddColumn( xCol )
                    yCol = vtkStringArray()
                    yCol.SetName( "y" )
                    convTable.AddColumn( yCol )
                    cCol = vtkIdTypeArray()
                    cCol.SetName( "Cardinality" )
                    convTable.AddColumn( cCol )

                    # Loop over all input rows and create output rows
                    nRow = readTable.GetNumberOfRows()
                    row = vtkVariantArray()
                    row.SetNumberOfValues( 4 )
                    for r in range( 0, nRow ):
                        # Retrieve primary statistics and convert to correct type
                        k = readTable.GetValueByName( r, "Key" ).ToInt()
                        row.SetValue( 0, k )
                        x = readTable.GetValueByName( r, "x" ).ToString()
                        row.SetValue( 1, x )
                        y = readTable.GetValueByName( r, "y" ).ToString()
                        row.SetValue( 2, y )
                        c = readTable.GetValueByName( r, "Cardinality" ).ToInt()
                        row.SetValue( 3, c )

                        convTable.InsertNextRow( row )

                # Set callback and run programmable filer
                convertContingencyTab.SetExecuteMethod( ConvertContingencyTableCallback )
                convertContingencyTab.Update()

                # Retrieve converted table from filter output
                inTable = convertContingencyTab.GetOutput()
                if verbosity > 1:
                    inTable.Dump( 16 )

            # Set retrieved table to corresponding model block
            inModel.SetBlock( t, inTable )

        # If model update is required, then learn new model and aggregate, otherwise assess directly
        if updateModel == True:
            # Store model it for subsequent aggregation
            collection = vtkDataObjectCollection()
            collection.AddItem( inModel )
            
            # Then learn a new primary model (do not derive nor assess)
            haruspex.SetLearnOption( True )
            haruspex.SetDeriveOption( False )
            haruspex.SetAssessOption( False )
            haruspex.Update()
            
            # Aggregate old and new models
            collection.AddItem( haruspex.GetOutputDataObject( 1 ) )
            aggregated = vtkMultiBlockDataSet()
            haruspex.Aggregate( collection, aggregated )

            # Finally, derive and possibly assess using the aggregated model (do not learn)
            haruspex.SetInput( 2, aggregated )
            haruspex.SetLearnOption( False )
            haruspex.SetDeriveOption( True )
            haruspex.SetAssessOption( assessOption )
            haruspex.Update()
        else:
            # Only derive and possibly assess using the input model (do not aggregate)
            haruspex.SetInput( 2, inModel )
            haruspex.SetLearnOption( False )
            haruspex.SetDeriveOption( True )
            haruspex.SetAssessOption( assessOption )
            haruspex.Update()
            
        print
############################################################

############################################################
# Main function
def main():
    # Parse command line
    [ inDataName, \
      inModelPrefix, \
      updateModel, \
      columnsListName, \
      haruspexName, \
      options, \
      outDataName, \
      outTestName, \
      outModelPrefix ] = ParseCommandLine()

    # Verify that haruspex name makes sense and if so instantiate accordingly
    haruspex = InstantiateStatistics( haruspexName )

    # Set input data reader
    inDataReader = ReadInData( inDataName )

    # Set input model readers if prefix was provided
    if inModelPrefix != "":
        inModelReader = []
        nPrimaryTables = haruspex.GetNumberOfPrimaryTables()
        for t in range( 0, nPrimaryTables ):
            tableReader = ReadInModelTable( inModelPrefix, t )
            inModelReader.append( tableReader )

    else:
        inModelReader = None
        
    # Read list of columns of interest
    if columnsListName:
        columnsList = ReadColumnsList( columnsListName )
    else:
        columnsList = []
        
    # Calculate statistics
    CalculateStatistics( inDataReader, inModelReader, updateModel, columnsList, haruspex, options )

    # Save output (annotated) data
    WriteOutTable( haruspex, 0, outDataName, "annotated data", 2 )

    # Save output of statistical tests
    WriteOutTable( haruspex, 2, outTestName, "statistical test results", 1 )

    # Save output model (statistics)
    WriteOutModel( haruspex, outModelPrefix )
############################################################

############################################################
if __name__ == "__main__":
    main()
############################################################
