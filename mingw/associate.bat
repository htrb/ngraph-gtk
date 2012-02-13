@echo off
ASSOC .ngp=NgraphGraph
FTYPE NgraphGraph="%CD%\bin\ngraph.exe" "%%1" %%*
bin\ngraph.exe -i echo.nsc "Instalation is finished."
