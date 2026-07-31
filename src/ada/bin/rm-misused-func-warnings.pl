#!/usr/bin/perl

# rm-misused-func-warnings.pl - Removes unavoidable warnings from linker output
# Copyright (C) 2026 Bill Chatfield
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

my $prevLine = "";

while (<>) {
    if (/ warning: \w*\(\) (is almost always misused, please use|is often misused, please use|is dangerous; do not use it)/) {
        # Don't print the previous line and make sure this line isn't printed either.
        $prevLine = "";
    } else {
        print $prevLine;
        $prevLine = $_;
    }
}

print $prevLine;

