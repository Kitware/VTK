#!/usr/bin/env perl
use warnings;
use strict;

my $fp;
open $fp, "<$ARGV[0]";

my $result = q{};
my $commentBlockStarted = 0;
my $functionStarted = 0;

for my $line (<$fp>) {
    # remove trailing new lines
    chomp $line;

    # skip empty lines
    if ($line =~ m{^\s*$}) {
        $result .= "\n";
        next;
    }

    # Inside comment block
    if ($commentBlockStarted == 1) {
        if ($line =~ m{#\]==\]}) {
            $line =~ s{#\]==\]}{*/};
            $commentBlockStarted = 0;
        }
        # replace bullets with '-', because '*' will be skipped by doxygen
        $line =~ s{^\s*\*\s+}{ - };
        $result .= "$line\n";
        next;
    }

    # Inside function/macro. Replace content with empty lines so that
    # definitions are at the correct line number.
    if ($functionStarted == 1) {
        if ($line =~ m{(^endfunction\s*\(.*\)|^endmacro\s*\(.*\))}) {
            $result .= "}\n";
            $functionStarted = 0;
            next;
        }
        $result .= "\n";
        next;
    }

    # Start comment block
    if ($line =~ m{#\[==\[}) {
        $line =~ s{#\[==\[}{/**};
        $result .= "$line\n";
        $commentBlockStarted = 1;
        next;
    }

    # Replace normal comment with empty line
    if ($line =~ m{^\s*#}) {
        $result .= "\n";
        next;
    }

    # Remove end-of-line comment
    $line =~ s{#.*}{};

    # Start function/macro
    if ($line =~ m{(^function\s*\()|(^macro\s*\()}) {
        $functionStarted = 1;
        if ($line =~ m{^function\s*\(}) {
            $result .= "function ";
            $line =~ s{^function\s*\(}{};
        }
        elsif ($line =~ m{^macro\s*\(}) {
            $result .= "macro ";
            $line =~ s{^macro\s*\(}{};
        }

        # first argument is the name of the function/macro,
        # then the arguments
        $line =~ s{\)\s*$}{};
        my @args = split(' ', $line);
        $result .= $args[0];
        shift(@args);
        $result .= " (";
        for my $arg (@args) {
            $result .= $arg;
            if ($arg ne $args[-1]) {
                $result .= " ,";
            }
        }
        $result .= ") {\n";
        next;
    }

    # All other lines, replace with empty lines
    $result .= "\n";
    next;
}

print $result;
