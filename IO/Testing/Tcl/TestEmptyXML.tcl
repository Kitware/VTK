package require vtk

wm withdraw .

# List of types and corresponding file extensions.
set types {
    {ImageData vti}
    {RectilinearGrid vtr}
    {StructuredGrid vts}
    {PolyData vtp}
    {UnstructuredGrid vtu}
}

# Test each writer/reader.
foreach pair $types {
    set type [lindex $pair 0]
    set ext [lindex $pair 1]
    vtk${type} input
    vtkXML${type}Writer writer
    writer SetFileName "empty${type}.${ext}"
    puts "Attempting ${type} write with no input."
    catch {writer Write}
    puts "Attempting ${type} write with empty input."
    writer SetInput input
    writer Write

    vtkXML${type}Reader reader
    reader SetFileName "empty${type}.${ext}"
    puts "Attempting read from file with empty ${type}."    
    reader Update

    vtkXMLP${type}Writer pwriter
    pwriter SetFileName "emptyP${type}.p${ext}"
    pwriter SetNumberOfPieces 1
    puts "Attempting P${type} write with no input."
    catch {pwriter Write}
    puts "Attempting P${type} write with empty input."
    pwriter SetInput input
    pwriter Write
    
    vtkXMLP${type}Reader preader
    preader SetFileName "emptyP${type}.p${ext}"
    puts "Attempting read from file with empty P${type}."    
    preader Update
    
    input Delete
    writer Delete
    reader Delete
    pwriter Delete
    preader Delete
}

# Delete the test files.
foreach pair $types {
    set type [lindex $pair 0]
    set ext [lindex $pair 1]
    file delete -force "empty${type}.${ext}"
    file delete -force "emptyP${type}.p${ext}"
    file delete -force "emptyP${type}0.${ext}"
}

exit
