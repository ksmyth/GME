#!/usr/bin/python
#
# Copyright (c) 2006 ISIS, Vanderbilt University
#
# Author: Peter Volgyesi (peter.volgyesi@vanderbilt.edu)
# Kevin Smyth (ksmyth@isis.vanderbilt.edu)
#
"""GME Build System - tools module"""

import os
import os.path
import sys
import subprocess
import win32com.client
import win32api
import time
from prefs import prefs

#
# Constants
#
ZIP_PRG = os.path.abspath(os.path.join(os.path.dirname(__file__), "zip.exe"))
WIX_CANDLE_PRG = "candle.exe"
WIX_CANDLE_ARG = "-dPIADir.1.0.0.0=..\GME\DotNetPIAs_1.0.0.0 -dPIADir.1.0.1.0=..\GME\DotNetPIAs_1.0.1.0 -dPIADir=..\GME\DotNetPIAs"
WIX_LIGHT_PRG = "light.exe"
WIX_LIGHT_ARG = "-sw1076 -sw1055 -sw1056 -sice:ICE43 -sice:ICE57 -ext WixUIExtension -ext WixUtilExtension -ext WiXNetFxExtension" # See comments in GME.wxs
MSBUILD = r"C:\Program Files (x86)\MSBuild\14.0\Bin\MSBuild.exe"
VCVARS = r"C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall.bat"
_this_dir = os.path.dirname(os.path.abspath(__file__))

#
# Classes
#
class BuildException(Exception):
    "General Exception Class for all build problems"
    pass

def Dispatch(progid):
    from pythoncom import CLSCTX_ALL, CLSCTX_LOCAL_SERVER
    CLSCTX_ACTIVATE_32_BIT_SERVER = 0x40000
    CLSCTX_ACTIVATE_64_BIT_SERVER = 0x80000
    if prefs['arch'] == 'x64':
        return win32com.client.DispatchEx(progid, clsctx=CLSCTX_LOCAL_SERVER|CLSCTX_ACTIVATE_64_BIT_SERVER)
    else:
        return win32com.client.DispatchEx(progid)
        # return win32com.client.DispatchEx(progid, clsctx=CLSCTX_LOCAL_SERVER|CLSCTX_ACTIVATE_32_BIT_SERVER)

#
# Tools/utilities
#
def toolmsg(str):
    """
    Displays log messages, used by other tool functions
    """
    if prefs['verbose']:
        print "\t" + str + "..."

def test_SVN():
    "Test for SVN client. Raises exception if not found."
    system(['svn', '--version', '>NUL'])


def test_zip():
    "Test for ZIP utility. Raises exception if not found."
    system([ZIP_PRG, '>NUL'])


def zip(dirname, zipname, list=None):
    """
    Build zip archive in the specified directory.
    The optional 'list' argument can specify the name of the file containing the
    include patterns for the archive.
    params
        dirname : the path to the root of the files to be archived
        zipname : the name of the zip archive (will be created in 'dirname')
        list    : name of the list file (see above)
    """
    cmd_line = [ZIP_PRG, '-9', '-r']
    if list is not None:
        cmd_line.append("-i@" + list)
    cmd_line.extend([zipname, '.', '>NUL'])
    system(cmd_line, dirname)

def collect_and_zip(dirname, zipname, pattern=None):
    """
    Collect files (recursively) and build zip archive in the specified directory.
    params
        dirname : the path to the root of the files to be archived
        zipname : the name of the zip archive (will be created in 'dirname')
        pattern : file name pattern for selecting files to be included
    """
    cmd_line = [ZIP_PRG, '-9', '-r', zipname, '.']
    if pattern is not None:
        cmd_line.extend(["-i"] + pattern.split())
    system(cmd_line, dirname)


def system(args, dirname=None):
    """
    Executes a system command (throws an exception on error)
    params
        args : [command, arg1, arg2, ...]
        dirname : if set, execute the command within this directory
    """
    toolmsg("Executing " + str(args))
    #toolmsg(" ".join(map(lambda x: '"' + x + '"', args)))
    with open(os.devnull, "w") as nulfp:
        # n.b. stderr=subprocess.STDOUT fails mysteriously
        subprocess.check_call(args, stdout=(sys.stdout if prefs['verbose'] else nulfp), stderr=subprocess.STDOUT, shell=True, cwd=dirname)


def test_VS():
    "Test for Microsoft Visual Studio 2015. Raises exception if not found."
    toolmsg("Trying to create VisualStudio.DTE object")
    win32com.client.Dispatch("VisualStudio.DTE.14.0")

def build_VS(sln_path, config_name, arch=None, msbuild=MSBUILD, target=None):
    """
    Builds a Microsoft Visual Studio project or entire solution.
    It cleans the project/solution before building it if the global 'clean' preference
    is set.
    params
        sln_path     : full path to the solution (.sln) file
        config_name  : name of the build configuration (e.g.: "Release")
    """
    msg = "Cleaning and " * prefs['clean']
    msg += "Compiling " + sln_path + "(" + config_name + ") "
    toolmsg(msg)
    arch = arch or prefs['arch']
    target = target or ("Clean;" * prefs['clean']) + 'Build'

    # , '/fl', '/flp:Verbosity=diagnostic'
    # , '/m'
    args = [msbuild, sln_path, '/nologo', '/m', '/t:' + target,
         '/p:VisualStudioVersion=%s.0;PlatformToolset=v%s0;Configuration=%s' % (prefs['toolset'], prefs['toolset'], config_name) +
        (';Platform=x64' if arch == 'x64' else ''),
        '/fl', '/flp:Verbosity=normal;LogFile=msbuild_{}_{}.log'.format(os.path.basename(sln_path), arch),
        '/clp:' + ('Verbosity=minimal' if prefs['verbose'] else 'Verbosity=quiet')]
    # n.b. stderr=subprocess.STDOUT fails mysteriously
    subprocess.check_call(args, stdout=sys.stdout, stderr=None, shell=True)

def xme2mga(xml_file, paradigm):
    """
    Generates an .mga file from an .xme file
    params
        xml_file    : full path to the xme file
        paradigm    : use the specified paradigm to parse the project

    The generated .mga file will be created with the same name/path but different
    extension.
    """
    toolmsg("Parsing " + xml_file + " with paradigm " + paradigm)
    parser  = Dispatch( "MGA.MgaParser" )
    project = Dispatch( "MGA.MgaProject" )
    mga_file = os.path.splitext(xml_file)[0] + ".mga"
    project.Create( "MGA="+mga_file, paradigm )
    parser.ParseProject( project, xml_file )
    project.Close()


def xmp2mta(xml_file, paradigm):
    """
    Generates and registers (system-wide) an .mta file from an .xmp file
    params
        xml_file    : full path to the xmp file
        paradigm    : use the specified paradigm name to parse the paradigm file

    The generated .mta file will be created with the same name/path but different
    extension.
    """
    toolmsg("Parsing and registering " + xml_file + " (" + paradigm + ")")
    registrar = Dispatch( "MGA.MgaRegistrar" )
    # KMS: registering user fails if system is already registered. TODO: remove so we dont need elevation
    if paradigm in registrar.GetParadigmsDisp(2):
        registrar.UnregisterParadigm(paradigm, 2)
    registrar.RegisterParadigmFromData( "XML=" + xml_file, paradigm, 1 )


def query_GUID(mta_file):
    """
    Queries the current GUID of the specified paradigm.
    params
        paradigm    : the name of the paradigm to be queried

    returns the GUID as a string
    """
    metaproject = Dispatch("MGA.MgaMetaProject")
    metaproject.Open('MGA=' + mta_file)
    try:
        import uuid
        return '{' + str(uuid.UUID(bytes_le=metaproject.GUID)).upper() + '}'
    finally:
        metaproject.Close()


def _get_wix_path():
    wix_version = '3.11.1'
    wix_dir = 'WiX.' + wix_version
    if not os.path.isdir(os.path.join(_this_dir, 'nuget', wix_dir)):
        subprocess.check_call([os.path.join(_this_dir, r'..\..\Tests\GPyUnit\DsmlGeneratorTest\.nuget\NuGet.exe')] +
            'install -Source https://www.nuget.org/api/v2/ -OutputDirectory nuget -Version'.split() + [wix_version, 'WiX'], cwd=_this_dir)
    return os.path.join(_this_dir, 'nuget', wix_dir, 'tools')


def test_WiX():
    "Test for WiX. Raises exception if not found."
    toolmsg("Trying to execute WiX tool candle.exe")
    exepath = os.path.join(_get_wix_path(), WIX_CANDLE_PRG)
    system([exepath])


def _x64_suffix(str):
    return str + '_x64' if prefs['arch'] == 'x64' else str
def _get_wixobj(file):
    return _x64_suffix(os.path.splitext(file)[0]) + ".wixobj"

def _candle(file):
    exepath = os.path.join(_get_wix_path(), WIX_CANDLE_PRG)
    cmd_line = [exepath] + WIX_CANDLE_ARG.split() + ['-dVERSIONSTR=' + prefs['version_string'], '-arch', prefs['arch'], '-out', _get_wixobj(file), file]
    system(cmd_line, os.path.dirname(file))

def build_WiX(wix_files):
    """
    Builds a WiX project.
    """
    fullpath = os.path.normpath(os.path.abspath(wix_files[0]))
    dirname = os.path.dirname(fullpath)
    filename = os.path.basename(fullpath)

    toolmsg("Building " + filename + " in " + dirname)
    wxi_files = filter(lambda file: file.find(".wxi") != -1, wix_files)
    mm_files = filter(lambda file: file.find(".wxs") != -1, wix_files)

    for file in wix_files:
        _candle(file)

    for wxs in mm_files:
        if wxs.find('GME.wxs') == -1:
            exepath = os.path.join(_get_wix_path(), 'lit.exe')
            ext = '.wixlib'
            wixlibs = ['-bf']
        else:
            exepath = os.path.join(_get_wix_path(), 'light.exe')
            ext = '.msi'
            wixlibs = WIX_LIGHT_ARG.split() + ['GME_bin.wixlib', 'GME_SDK.wixlib', 'GME_paradigms.wixlib']
            if prefs['arch'] == 'x64':
                wixlibs += ['GME_bin_x64.wixlib', 'GME_SDK_x64.wixlib']
        cmd_line = [exepath] + ['-o', _x64_suffix(os.path.splitext(wxs)[0]) + ext] + [ _get_wixobj(file) for file in wxi_files ] + [ _get_wixobj(wxs)] + wixlibs
        system(cmd_line, dirname)


import errno
import requests
import tempfile
import hashlib
import itertools
from xml.etree import ElementTree
import xml.sax
from xml.sax.handler import ContentHandler


def sha1_filename(filename):
    import hashlib
    m = hashlib.sha1()
    with open(filename, 'rb') as input:
        while True:
            chunk = input.read(4096 * 20)
            if not chunk:
                break
            m.update(chunk)
    return m.hexdigest()


def download_file(url, filename, expected_sha1):
    headers = {}
    if os.path.isfile(filename):
        mtime = time.gmtime(os.stat(filename).st_mtime)
        timestamp = time.strftime('%a, %d %b %Y %H:%M:%S GMT', mtime)
        headers["If-Modified-Since"] = timestamp

    print('Downloading {} => {} ({})'.format(url, filename, expected_sha1))
    if os.path.dirname(filename):
        try:
            os.makedirs(os.path.dirname(filename))
        except OSError as e:
            if e.errno != errno.EEXIST:
                raise
    r = requests.get(url, headers=headers, stream=True)
    r.raise_for_status()
    if r.status_code == 304:
        print('  Skipping {}: Last-Modified {}'.format(filename, r.headers.get('Last-Modified')))
        sha1 = sha1_filename(filename)
        if expected_sha1 != sha1:
            raise ValueError('File at {} changed. Update @Name and expected_sha1 (expect {} vs actual {})'.format(url, expected_sha1, sha1))
        return
    fd, tmp_path = tempfile.mkstemp()
    # wix bootstrapper uses SHA1
    hash = hashlib.sha1()
    with os.fdopen(fd, 'wb') as f:
        for chunk in r.iter_content(chunk_size=1024):
            if chunk:  # filter out keep-alive new chunks
                hash.update(chunk)
                f.write(chunk)
        # n.b. don't use f.tell(), since it will be wrong for Content-Encoding: gzip
        downloaded_octets = r.raw._fp_bytes_read
    if int(r.headers.get('content-length', downloaded_octets)) != downloaded_octets:
        os.unlink(tmp_path)
        raise ValueError('Download of {} was truncated: {}/{} bytes'.format(url, downloaded_octets, r.headers['content-length']))
    else:
        os.rename(tmp_path, filename)
        print('  => {} {}'.format(filename, hash.hexdigest()))
        if expected_sha1 != hash.hexdigest():
            raise ValueError('File at {} changed. Update @Name and expected_sha1'.format(url))


class WixProcessingInstructionHandler(ContentHandler):
    def __init__(self):
        ContentHandler.__init__(self)
        self.defines = {}

    def processingInstruction(self, target, data):
        if target == 'define':
            eval(compile(data, '<string>', 'exec'), globals(), self.defines)
        elif target == 'include':
            pass  # TODO


def get_version_number(filename):
    info = win32api.GetFileVersionInfo(filename, "\\")
    ms = info['FileVersionMS']
    ls = info['FileVersionLS']
    return win32api.HIWORD(ms), win32api.LOWORD(ms), win32api.HIWORD(ls), win32api.LOWORD(ls)


def download_bundle_deps(bundle_wxs, define_files=[]):
    bundle_dir = os.path.dirname(os.path.abspath(bundle_wxs))
    downloaded_files = []
    defines = WixProcessingInstructionHandler()
    for define_file in define_files:
        assert os.path.isfile(define_file)
        print(define_file)
        xml.sax.parse(define_file, defines)

    def eval_vars(attr):
        for name, val in defines.defines.iteritems():
            attr = attr.replace('$(var.{})'.format(name), val)
        return attr

    tree = ElementTree.parse(bundle_wxs).getroot()
    ElementTree.register_namespace("", "http://schemas.microsoft.com/wix/2006/wi")

    sha1s = {
        "redist\\vc_redist.x86.v14.24.28127.4.exe": "AFA5BADCE64EE67290ADD24E0DC3D8210954AC6C",
        "redist\\Windows6.1-KB2999226-x64.msu": "54050A5F8AE7F0C56E553F0090146C17A1D2BF8D",
        "redist\\Windows8.1-KB2999226-x64.msu": "3ACBF3890FC9C8A6F3D2155ECF106028E5F55164",
        "redist\\Windows6.0-KB2999226-x64.msu": "0BEFBB0B78588F7C9F17EAD1DA3ABEDA2B6F4C7F",
        "redist\\vc_redist.x64.v14.24.28127.4.exe": "B87C38D093872D7BE7E191F01107B39C87888A5A",
        "redist\\Windows8-RT-KB2999226-x64.msu": "B5943B2700B56F5F8DC307A9E237F23FCA5D8B70",
    }
    for package in itertools.chain(tree.findall(".//{http://schemas.microsoft.com/wix/2006/wi}ExePackage"),
            tree.findall(".//{http://schemas.microsoft.com/wix/2006/wi}MsuPackage"),
            tree.findall(".//{http://schemas.microsoft.com/wix/2006/wi}MsiPackage")):
        url = eval_vars(package.get('DownloadUrl', ''))
        if not url:
            continue
        base_filename = eval_vars(package.get('SourceFile', '') or package.get('Name', ''))
        filename = os.path.join(bundle_dir, base_filename)
        download_file(url, filename, sha1s[eval_vars(package.get('Name', ''))].lower())
        downloaded_files.append(filename)

        if package.tag.endswith('ExePackage'):
            version_map = {
                'vcredist_2015': 'VcRedistVersion',
                }
            for filename_prefix, version in version_map.items():
                if package.get('Id').startswith(filename_prefix):
                    defines_version = list(map(int, defines.defines[version].replace('v', '').split('.')))
                    if defines_version != list(get_version_number(filename)):
                        raise ValueError('{}: Expected version {!r} does not match downloaded version {!r}'.format(package.get('Id'), defines_version, list(get_version_number(filename))))
                    break
            else:
                raise ValueError('No entry in version_map for {}'.format(package.get('Id')))

    # from https://github.com/wixtoolset/wix3/blob/develop/src/ext/NetFxExtension/wixlib/NetFx4.5.wxs
    # filename = 'redist\\dotNetFx45_Full_setup.exe'
    # download_file('http://go.microsoft.com/fwlink/?LinkId=225704', filename)
    # downloaded_files.append(filename)
    return downloaded_files
