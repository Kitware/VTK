#!/usr/bin/tclsh

# This script will find all files that include certain regular expression.
# If the files are not in the list provided, the script will return error.

set ProgName [ lindex [ split $argv0 "/" ] end ]

if { $argc < 2 } {
    puts "Usage: $ProgName <expr1> <expr2> \[ <file> ... \]"
    puts "\texpr1 - file list expression (vtk*.h)"
    puts "\texpr2 - search string expression (vtkSet.*Macro)"
    puts "\tfile  - files that should be ignore"

    puts ""
    puts "You provided:"
    foreach { a } $argv {
	puts "$a"
    }

    exit 1
}

# Parse command line arguments
set FileExpression [ lindex $argv 0 ]
set SearchMessage  [ lindex $argv 1 ]
set IgnoreFileListIn [ lrange $argv 2 end ]
set IgnoreFileList {}
foreach { file } $IgnoreFileListIn {
   set IgnoreFileList "$IgnoreFileList [ glob $file ]"
}
#puts "Searching for $SearchMessage in $FileExpression"
#puts "Ignore list: $IgnoreFileList"

# Find regular expression in the string
proc FindString { InFile SearchString } {
    if [ catch { open $InFile r } inchan ] {
	puts stderr "Cannot open $InFile"
	return 0
    }
    set res 0
    set lcount 1
    while { ! [eof $inchan] } {
	gets $inchan line
	if [ regexp $SearchString $line matches ] {
	    puts "$InFile: Found $SearchString on line $lcount"
	    puts "$line"
	    set res 1
	}
	set lcount [ expr $lcount + 1 ]
    }
    close $inchan
    return $res
}

# Get all files that match expression
set files ""
if [ catch { [ set files [ glob $FileExpression ] ] } result ] {
    regsub {\\\*} $FileExpression "*" FileExpression
    if [ catch { [ set files [ glob $FileExpression ] ] } nresult ] {
	#puts "Cannot expand the expression: \"$FileExpression\""
	#puts "Error: $nresult"
        #exit 1
    }
}

if { [ llength $files ] < 1 } {
    puts "Cannot find any files that match your file expression"
    exit 0
}

set count 0
foreach { a } $files {
   regsub -all {\\} $a {/} b 
   if { [ lsearch $IgnoreFileList $b ] >= 0 } {
	puts "Ignoring: $b"
    } else {
	set count [ expr $count + [ FindString $a $SearchMessage ] ]
    }
}

if { $count > 0 } {
    puts "" 
    puts "Found \"$SearchMessage\" $count times"
    exit 1
}

exit 0
