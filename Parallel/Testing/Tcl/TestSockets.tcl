exec [lindex $argv 0] -D [lindex $argv 2] &
set [catch {exec [lindex $argv 1] -D [lindex $argv 2] -V [lindex $argv 3]}] errs
if {[info exists errs]} {
    puts stderr "Sockets test failed: $errs"
}