package require vtk

# List of types and corresponding file extensions.
set types {
    {ImageData vti}
    {RectilinearGrid vtr}
    {StructuredGrid vts}
    {PolyData vtp}
    {UnstructuredGrid vtu}
}

# Prepare some test files.
file delete -force "junkFile.vtk"
file delete -force "emptyFile.vtk"
set f [open "emptyFile.vtk" w]
close $f
set f [open "junkFile.vtk" w]
puts $f "v9np7598mapwcawoiur-,rjpmW9MJV28nun-q38ynq-9 8ugujqvt-8n3-nv8"
close $f

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

    reader SetFileName "emptyFile.vtk"
    preader SetFileName "emptyFile.vtk"

    puts "Attempting read ${type} from empty file."
    reader Update
    puts "Attempting read P${type} from empty file."
    preader Update

    reader SetFileName "junkFile.vtk"
    preader SetFileName "junkFile.vtk"

    puts "Attempting read ${type} from junk file."
    reader Update
    puts "Attempting read P${type} from junk file."
    preader Update
    
    input Delete
    writer Delete
    reader Delete
    pwriter Delete
    preader Delete
}

# Test the data set writers.
foreach pair $types {
    set type [lindex $pair 0]
    set ext [lindex $pair 1]
    vtkXMLDataSetWriter writer
    vtkXMLPDataSetWriter pwriter
    vtk${type} input

    writer SetFileName "empty${type}DataSet.${ext}"
    puts "Attempting DataSet ${type} write with no input."
    catch {writer Write}
    puts "Attempting DataSet ${type} write with empty input."
    writer SetInput input
    writer Write

    pwriter SetFileName "emptyP${type}DataSet.p${ext}"
    pwriter SetNumberOfPieces 1
    puts "Attempting DataSet P${type} write with no input."
    catch {pwriter Write}
    puts "Attempting DataSet P${type} write with empty input."
    pwriter SetInput input
    pwriter Write

    input Delete
    pwriter Delete
    writer Delete
}

# Delete the test files.
foreach pair $types {
    set type [lindex $pair 0]
    set ext [lindex $pair 1]
    file delete -force "empty${type}.${ext}"
    file delete -force "empty${type}DataSet.${ext}"
    file delete -force "emptyP${type}.p${ext}"
    file delete -force "emptyP${type}0.${ext}"
    file delete -force "emptyP${type}DataSet.p${ext}"
    file delete -force "emptyP${type}DataSet0.${ext}"
}
file delete -force "junkFile.vtk"
file delete -force "emptyFile.vtk"

exit
