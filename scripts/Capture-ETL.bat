@echo off
pushd %~dp0
wpr -start VirtualDesktopOpenXR.wprp -filemode

echo Reproduce your issue now, then
pause

wpr -stop VDXR.etl
popd
