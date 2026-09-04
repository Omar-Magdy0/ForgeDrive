set pagination off
set print pretty on
set print object on
set print static-members on
set print vtbl on
set print demangle on
set demangle-style gnu-v3
set print array on
set print array-indexes on
set print elements 0
set print null-stop
set print repeats 0
set print symbol-filename on
set verbose on
set history save on
set history filename ~/.gdb_history
set history size 10000

target extended-remote /dev/ttyBmpGdb
monitor swdp_scan
attach 1
set mem inaccessible-by-default off
file build/bin/EcoJM.elf
load
