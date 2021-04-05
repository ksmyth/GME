Setlocal EnableDelayedExpansion

set PATH=c:\Program Files\Git\usr\bin;%PATH%;c:\Program Files\7-Zip;c:\cygwin\bin;C:\Program Files (x86)\Subversion\bin
if not exist "%PYTHON%" set PYTHON=%userprofile%\Downloads\markdown\Scripts\python.exe
if not exist "%PYTHON_x64%" set PYTHON_x64=c:\Python27-64\python.exe

rem where "%PYTHON%" || exit /b !ERRORLEVEL!
rem pip install pywin32
"%PYTHON%" -c "import win32com.client" || exit /b !ERRORLEVEL!
"%PYTHON_x64%" -c "import win32com.client" || exit /b !ERRORLEVEL!
"%PYTHON%" -c "import requests" || exit /b !ERRORLEVEL!
"%PYTHON_x64%" -c "import requests" || exit /b !ERRORLEVEL!
"%PYTHON%" -c "import markdown" || exit /b !ERRORLEVEL!
rem python should be 2.7 x86
where 7z.exe || exit /b !ERRORLEVEL!
find -maxdepth 0 || exit /b !ERRORLEVEL!
dir "%GME_ROOT%" || exit /b !ERRORLEVEL!
if not exist "%SYMSTORE%" set SYMSTORE=C:\Program Files (x86)\Windows Kits\10\Debuggers\x64\symstore.exe
dir "%SYMSTORE%" || exit /b !ERRORLEVEL!

if exist redist echo Move redist out of the way
if exist redist exit /b 1

rem C:\Program Files (x86)\Debugging Tools for Windows (x86)\symstore.exe
rem C:\Program Files (x86)\Windows Kits\8.0\Debuggers\x64\symstore.exe
rem C:\Program Files (x86)\Windows Kits\8.0\Debuggers\x86\symstore.exe

pushd %GME_ROOT%

for /f "usebackq" %%a in (`%python% -c "import time; print '.'.join(map(str, time.localtime()[0:3]))[2:]"`) do set VERSION=%%a
echo Version %VERSION%

"%PYTHON%" Install\Build\build.py -s 1 -e build_msi -v || exit /b !ERRORLEVEL!
"%PYTHON_x64%" Install\Build\build.py -s 1 -e build_msms -v -a x64 || exit /b !ERRORLEVEL!
"%PYTHON%" Install\Build\build.py -s build_msi -e build_msi -v -a x64 || exit /b !ERRORLEVEL!

mkdir redist

find Install -iname \*.wixlib -exec cp "{}" redist ";" || exit /b !ERRORLEVEL!
find Install -iname \*.msi -exec cp "{}" redist ";" || exit /b !ERRORLEVEL!
find Install -iname GME\*.exe -exec cp "{}" redist ";" || exit /b !ERRORLEVEL!

rem export https://svn.isis.vanderbilt.edu/GMESRC/trunk GME-%VERSION%_src || exit /b !ERRORLEVEL!
rem xe a GME-%VERSION%_src.7z GME-%VERSION%_src/ || exit /b !ERRORLEVEL!
git archive -o GME-src.zip HEAD || exit /b !ERRORLEVEL!
rd /s/q GME-%VERSION%_src
7z x -oGME-%VERSION%_src GME-src.zip || exit /b !ERRORLEVEL!
7z a GME-%VERSION%_src.7z GME-%VERSION%_src || exit /b !ERRORLEVEL!

move /y GME-%VERSION%_src.7z redist || exit /b !ERRORLEVEL!

move redist\GME.msi redist\GME-%VERSION%.msi || exit /b !ERRORLEVEL!
move redist\GME_x64.msi redist\GME_x64-%VERSION%.msi || exit /b !ERRORLEVEL!
move redist\GME_bundle.exe redist\GME-%VERSION%.exe || exit /b !ERRORLEVEL!

mkdir GME-%VERSION%_symbols
"%SYMSTORE%" add /r /f "%GME_ROOT%" /s GME-%VERSION%_symbols /t GME || exit /b !ERRORLEVEL!
find GME-%VERSION%_symbols -type f -iname vc100.pdb -delete || exit /b !ERRORLEVEL!
find GME-%VERSION%_symbols -type f -iname vc140.pdb -delete || exit /b !ERRORLEVEL!
find GME-%VERSION%_symbols -type f ^^! -iname "*.pdb" -delete || exit /b !ERRORLEVEL!
find GME-%VERSION%_symbols -type d -empty -delete || exit /b !ERRORLEVEL!

"c:\Program Files\7-Zip\7z.exe" a -xr^^!.svn redist\GME-%VERSION%_symbols.7z GME-%VERSION%_symbols || exit /b !ERRORLEVEL!

echo Success

exit /b 0

if not exist "%PYTHON%" set PYTHON=%userprofile%\Downloads\markdown\Scripts\python.exe
for /f "usebackq" %a in (`%python% -c "import time; print '.'.join(map(str, time.localtime()[0:3]))[2:]"`) do set VERSION=%a
echo Version %VERSION%

set PATH=%PATH%;c:\Program Files\7-Zip;c:\Program Files\Git\usr\bin;c:\cygwin\bin;C:\Program Files (x86)\Subversion\bin

rem where svn.exe || exit /b !ERRORLEVEL!
rem svn ls https://svn.isis.vanderbilt.edu/GMESRC || exit /b !ERRORLEVEL!
svn cp -m "Tag Release %VERSION%" https://svn.isis.vanderbilt.edu/GMESRC/trunk https://svn.isis.vanderbilt.edu/GMESRC/tags/Release_%VERSION%

cd redist
ssh repo@repo.isis.vanderbilt.edu mkdir GME/%VERSION%/ && \
scp ../Doc/README.txt GME-%VERSION%.exe GME-%VERSION%.msi GME_x64-%VERSION%.msi GME-%VERSION%_src.7z repo@repo.isis.vanderbilt.edu:GME/%VERSION%/ && \
scp GME-%VERSION%_symbols.7z repo@repo.isis.vanderbilt.edu:GME/%VERSION%/ && \
scp ../Doc/index.html repo@repo.isis.vanderbilt.edu:GME/ && \
ssh repo@repo.isis.vanderbilt.edu chmod -R a+r GME/%VERSION%/

pushd %GME_ROOT%\Doc
%userprofile%\Downloads\markdown\scripts\python %userprofile%\Downloads\markdown\upload.py

