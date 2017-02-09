@echo off
ASSOC .ngp=NgraphGraph
FTYPE NgraphGraph="%CD%\bin\ngraph.exe" "%%1" %%*
