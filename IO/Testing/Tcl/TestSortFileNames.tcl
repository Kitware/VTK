package require vtk

vtkGlobFileNames globFileNames
globFileNames AddFileNames "$VTK_DATA_ROOT/Data/headsq/quarter.*"
globFileNames AddFileNames "$VTK_DATA_ROOT/Data/track*.binary.vtk"

vtkSortFileNames sortFileNames
sortFileNames SetInputFileNames [globFileNames GetFileNames]
sortFileNames NumericSortOn
sortFileNames SkipDirectoriesOn
sortFileNames IgnoreCaseOn
sortFileNames GroupingOn

if { [sortFileNames GetNumberOfGroups] != 2 } {
    puts "GetNumberOfGroups returned incorrect number"
    exit 1
}

set fileNames1 [sortFileNames GetFileNames 0]
set fileNames2 [sortFileNames GetFileNames 1]

set numberOfFiles1 93
set numberOfFiles2 3

set n [$fileNames1 GetNumberOfValues]
if { $n != $numberOfFiles1 } {
    puts "GetNumberOfValues should return $numberOfFiles1, not $n"
    exit 1
}

for { set i 0 } { $i < $numberOfFiles1 } { incr i } {
    set j [expr $i + 1]
    if {[$fileNames1 GetValue $i] !=
        "$VTK_DATA_ROOT/Data/headsq/quarter.$j"} {
        puts "string does not match pattern"
        puts [$fileNames1 GetValue $i]
        puts "$VTK_DATA_ROOT/Data/headsq/quarter.$j"
        exit 1
    }
}

set n [$fileNames2 GetNumberOfValues]
if { $n != $numberOfFiles2} {
    puts "GetNumberOfValues should return $numberOfFiles2, not $n"
    exit 1
}

for { set i 0 } { $i < $numberOfFiles2 } { incr i } {
    set j [expr $i + 1]
    if {[$fileNames2 GetValue $i] !=
        "$VTK_DATA_ROOT/Data/track$j.binary.vtk"} {
        puts "string does not match pattern"
        puts [$fileNames2 GetValue $i]
        puts "$VTK_DATA_ROOT/Data/track$j.binary.vtk"
        exit 1
    }
}

globFileNames Delete
sortFileNames Delete

exit
