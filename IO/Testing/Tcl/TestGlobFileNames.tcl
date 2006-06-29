package require vtk

vtkGlobFileNames globFileNames
globFileNames AddFileNames "$VTK_DATA_ROOT/Data/headsq/quarter.*"
set fileNames [globFileNames GetFileNames]

set n [$fileNames GetNumberOfValues]
if { $n != 93 } {
    for { set i 0 } { $i < $n } { incr i } {
        puts [$fileNames GetValue $i]
    }
    puts "GetNumberOfValues should return 93, returned $n"
    exit 1
}

for { set i 0 } { $i < $n } { incr i } {
    set filename [$fileNames GetValue $i]
    if { "$filename" != [globFileNames GetNthFileName $i] } {
        puts "mismatched filename for pattern quarter.*: $filename"
        exit 1
    }
    if { [string match "*quarter.*" "$filename"] == 0 } {
        puts "strings does not match pattern quarter.*: $filename"
        exit 1
    }
}

# check that we can re-use the Glob object
globFileNames Reset
globFileNames AddFileNames "$VTK_DATA_ROOT/Data/financial.*"
set fileNames [globFileNames GetFileNames]

set n [$fileNames GetNumberOfValues]
for { set i 0 } { $i < $n } { incr i } {
    set filename [$fileNames GetValue $i]
    if { "$filename" != [globFileNames GetNthFileName $i] } {
        puts "mismatched filename for pattern financial.*: $filename"
        exit 1
    }
    if { [string match "*financial.*" $filename] == 0 } {
        puts "strings does not match pattern financial.*: $filename"
        exit 1
    }
}

globFileNames Delete

exit
