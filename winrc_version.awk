#!/usr/bin/awk

BEGIN {
    n = gsub(/[.]/, ",", VERSION);
    if (n == 2) {
	VERSION = sprintf("%s,00", VERSION);
    }
    gsub(/,0/, ",", VERSION);
}
{
    gsub("%VERSION%", VERSION);
    print
}
