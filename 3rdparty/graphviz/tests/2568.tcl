#!/usr/bin/wish
# inspired from https://wiki.tcl-lang.org/page/Tcldot
package require Tcldot
set g [dotstring {
   digraph G {
   dir1 -> CVS;
   dir1 -> src -> doc;
   dir1 -> vfs-> bin;
   vfs -> config -> ps;
   config -> pw -> pure;
   vfs -> step1 -> substep;
   dir1 -> www -> cgi;
   }
 }]
puts [$g render]
puts [$g listnodes]
