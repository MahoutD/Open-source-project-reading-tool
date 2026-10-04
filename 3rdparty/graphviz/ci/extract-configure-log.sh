#!/usr/bin/awk -f

/will be compiled with the following:/,/^  gdtclft:.*$/ {
    enable = 1;
}
{
    if (enable) {
        print $0;
    }
}
/^  gdtclft:.*$/ {
    enable = 0;
}
