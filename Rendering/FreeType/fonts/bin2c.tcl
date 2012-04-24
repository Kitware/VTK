proc convert_bin_to_c {bin_file_name} {

    # Get binary file size

    if {[catch {file size $bin_file_name} bin_file_size]} {
        puts stderr "Error: unable to get size of $bin_file_name"
        exit
    }

    puts "$bin_file_name is $bin_file_size bytes long."

    # Open binary file

    if {[catch {open $bin_file_name r} bin_file_id]} {
        puts stderr "Error: unable to open $bin_file_name"
        exit
    }

    puts "$bin_file_name is open for reading."

    # Read contents of binary file

    fconfigure $bin_file_id -translation binary

    if {[catch {read $bin_file_id} contents]} {
        puts stderr "Error: unable to read contents of $bin_file_name"
        exit
    }

    set contents_length [string length $contents]
    puts "$bin_file_name contents is stored ($contents_length bytes)."
    if {$contents_length != $bin_file_size} {
        puts stderr "Error: file size does not match length of contents"
        exit
    }

    # Close binary file

    if {[catch {close $bin_file_id}]} {
        puts stderr "Error: unable to close $bin_file_name"
    }

    puts "$bin_file_name is closed."

    # Open C file

    set bin_file_rootname [file rootname $bin_file_name]
    set bin_file_tail [file tail $bin_file_rootname]
    set c_file_name "${bin_file_rootname}.cxx"

    if {[catch {open $c_file_name w} c_file_id]} {
        puts stderr "Error: unable to open $c_file_name"
        exit
    }

    puts "$c_file_name is open for writing."

    # Convert bin to C

    puts "$c_file_name is filled."

    puts $c_file_id "size_t face_${bin_file_tail}_buffer_length = $contents_length;\n"

    puts -nonewline $c_file_id "unsigned char face_${bin_file_tail}_buffer\[\] = {\n  "

    for {set i 0} {$i < $contents_length} {incr i} {
        scan [string index $contents $i] "%c" byte
        puts -nonewline $c_file_id [format "%3d, " $byte]
        if {[expr $i % 14] == 13} {
            puts -nonewline $c_file_id "\n  "
        }
    }

    puts $c_file_id "\n};"

    # Close C file

    if {[catch {close $c_file_id}]} {
        puts stderr "Error: unable to close $c_file_name"
    }

    puts "$c_file_name is closed."
}

if {$argc < 1} {
    puts stderr "Error: expecting name of the binary file to convert"
    exit
}

foreach arg $argv {
    foreach file [glob -nocomplain $arg] {
        convert_bin_to_c $file
    }
}
