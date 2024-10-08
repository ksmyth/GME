              Generic Modeling Environment
       Copyright © 2000-2021 Vanderbilt University


************************************************
0. Important Upgrade Information
************************************************

The GME-x.y.z.exe installer includes both 64 bit and
32 bit GME. 64 bit GME does not support Multi-user projects.

.mga files saved with GME r11.12.2 and later cannot
be opened with previous versions of GME.
GME r11.12.2 and later can open .mga files saved with
previous versions of GME.

GME r11.12.2 uses a different format for the Multi-user
backend. Please export your Multi-user projects to .xme and
reimport after upgrading GME.

You must uninstall GME versions prior to version
9.1.12 using the Control Panel before installing GME.

************************************************
1. Release Notes
************************************************

Release Notes of Release 24.10.9
----------------------------------
  - Binary compatibility with 11.12.2
  - Fix crash in model editor (more likely under x64 GME)
  - Model Editor: fix right-click on tab menu showing up when clicking on other frames

Release Notes of Release 22.3.17
----------------------------------
  - Binary compatibility with 11.12.2
  - Render connection customization for high-dpi
  - Add right-click menu to model editor tabs
  - Fix dragging to resize FCO under high-dpi
  - Upgrade XmlBackend to SVN 1.14.1

Release Notes of Release 21.10.5
----------------------------------
  - Binary compatibility with 11.12.2
  - Fix high-DPI rendering for selection and resize in model editor

Release Notes of Release 21.4.1
----------------------------------
  - Binary compatibility with 11.12.2
  - Fix bug where 'Do you want to save?' dialog appears when the project hasn't been modified
  - Fix some high-DPI rendering issues

Release Notes of Release 20.2.12
----------------------------------
  - Binary compatibility with 11.12.2
  - Fix model editor right-click>Annotations crash
  - XME serialization: sort by GUID for containment, role for connections
  - Fix bug where right side of model editor wasn't rendered after increasing screen resolution
  - Java interpreter: don't crash when trying to run Java 10 .class with Java 8

Release Notes of Release 18.10.22
----------------------------------
  - Binary compatibility with 11.12.2
  - Model editor: fix scrolling with mousewheel upwards (affected only some systems)
  - Fix hang when a model gets bigger (e.g. adding ports) and the parent is open and has many connections
  - Console: allow javascript:(new ActiveXObject('shell.application')) links
  - Search: fix crash on searching for referred object after project was closed
  - Right-click drag-n-drop on Model Editor: grey out options that are disallowed by metamodel

Release Notes of Release 18.4.9
----------------------------------
  - Binary compatibility with 11.12.2
  - C# AddOn wizard: fix deactivating addon when restarting addons
  - CSharp Dsml Generator for GME x64
  - Interpreter wizard: fix x64 configuration
  - Interpreter wizard: Visual Studio 2017 support

Release Notes of Release 17.12.6
----------------------------------
  - Binary compatibility with 11.12.2
  - Fix rare crash when IMgaAddOn is garbage collected (e.g. C# AddOns)
  - Fix rare crash when calling Mga from a different COM apartment

Release Notes of Release 17.11.14
----------------------------------
  - Binary compatibility with 11.12.2
  - Fix crash under GME x64 and high memory usage
  - IMgaProject.ObjectByID: fail when there is no object with the supplied ID
  - DetachFromArchetype: fix bug where attribute values from basetype's basetype were lost

Release Notes of Release 17.10.13
----------------------------------
  - Binary compatibility with 11.12.2
  - Fix bug where deleting multiple objects in the tree browser didn't work
  - Some fixes for high-DPI displays

Release Notes of Release 17.9.28
----------------------------------
  - Binary compatibility with 11.12.2
  - Reject DetachFromArchetype for non-primary derived
  - Some fixes for high-DPI displays
  - Fix memory leak

Release Notes of Release 17.7.18
----------------------------------
  - Binary compatibility with 11.12.2
  - Add setup14.js for Visual Studio 2015 interpreter wizard
  - Fix possible crash when using C#
  - Fix multi-threaded XME parsing
  - MetaDecorator: don't allow editing of attributes in libraries
  - TreeBrowser: sort by RelID for FCOs with the same name

Release Notes of Release 17.3.13
----------------------------------
  - Binary compatibility with 11.12.2
  - Fix bug where instances or subtypes may have RelID=0x08000000
      To fix your model: export to xme and reimport
  - DsmlGenerator: fix bug where creating FCOs with a role may create with the wrong role
  - CSharpComponentWizard: fix "server busy" error message
  - CSharpComponentWizard: VS2015 support
  - Search: fix crash when Search within Scope is selected after the selected object is deleted

Release Notes of Release 16.7.22
----------------------------------
  - Binary compatibility with 11.12.2
  - Support Visual Studio 2015 in interpreter wizard
  - Add "Delete Port" option to right-click menu
  - Prevent DetachFromArchetype for targets of derived references
  - Fix bug with importing XME when a derived Reference refers to a different FCO than its base
  - Fix memory leak when deleting objects
  - Add IDispatch COM interface to interpreters

Release Notes of Release 16.3.23
----------------------------------
  - Binary compatibility with 11.12.2
  - Fix GME Console for high-DPI displays
  - BON: add CBuilderFolder::SetName
  - Fix hotspots for quick connect mode when scrolled or zoomed
  - Component window: fix bug where .NET interpreters with long paths would incorrectly be listed as GAC
  - Other bugfixes

Release Notes of Release 15.5.8
----------------------------------
  - Binary compatibility with 11.12.2
  - Fix bug where an instance of a library object would have IsLibObject == true after DetachFromArcheType() was called on it
  - Fix really long cropped annotations
  - Fix bug where floating-point numbers in xmes files were not read correctly under non-English locales (error "Type mismatch")
  - Default decorator does not stretch icons.

Release Notes of Release 14.12.4
----------------------------------
  - Binary compatibility with 11.12.2
  - Fix regression since 14.10.8 where .mga files opened with GME (64 bit). .mga files now correctly open with GME (32 bit)
  - Associate ConstraintManager with newly-registered paradigms (GME-446)
  - DsmlGenerator: include ISIS.GME.Common.dll 1.0.4.0 in installer
  - Drag-n-drop reference switch with refports: fix bug with R2->R1->M1 chain when switching R1 and R2 had refport connections
  - Fix icon for C++ dlls compiled with VS2012+

Release Notes of Release 14.10.29
----------------------------------
  - Binary compatibility with 11.12.2
  - CSharpComponentWizard: support for Visual Studio 2013
  - VC component wizard: support for Visual Studio 2013
  - Fix bug with addon notification where addons wouldn't get notified under certain circumstances, like registry edits

Release Notes of Release 14.10.10
----------------------------------
  - Binary compatibility with 11.12.2
  - Fix bug in installer where environment variable GME_ROOT wasn't set

Release Notes of Release 14.10.8
----------------------------------
  - Binary compatibility with 11.12.2
  - Interpreter wizard: generate interpreters that can be called from within a transaction
  - JavaCompRunner: support being called from within a transaction
  - gme.jar: expose more Mga methods
  - Search: allow multiselect

Release Notes of Release 14.7.28
----------------------------------
  - Binary compatibility with 11.12.2
  - Fix bug where 'Do you want to save?' dialog does not appear sometimes
  - Decorators: disable editing of labels in libraries
  - Fix unit test crash caused by ConstraintManager and a constraint violation
  - Upgrade SVN libraries for multiuser projects
  - Fix use-after-free bug triggered under scripts that reuse IMgaParser and fail to parse (GME-441)
  - Fix crash for deleted_object.AbsPath

Release Notes of Release 14.5.5
----------------------------------
  - Binary compatibility with 11.12.2
  - Drag-n-drop to Search pane to search for 'what references this'.
  - MetaInterpreter: detect duplicate names for abstract FCOs
  - Fix ObjectByPath: relpos was ignored sometimes
  - DSMLGenerator: fix for some namespace name clashes
  - Add newer Parser.idl interfaces to installer. Fixes QueryInterface calls for out-of-proc IMgaDumper2, IMgaParser2, and IMgaParser3

Release Notes of Release 14.3.5
----------------------------------
  - Binary compatibility with 11.12.2
  - Ctrl-Shift-V: paste as reference
  - For both Right-click on connection, Jump to source (or destination); and Jump along Incoming (or Outgoing) connection: highlight connection that is followed

Release Notes of Release 14.2.24
----------------------------------
  - Binary compatibility with 11.12.2
  - Fix bug where the GME Browser and Object Inspector would try to read a deleted FCO after an AddOn modified it

Release Notes of Release 14.2.19
----------------------------------
  - Binary compatibility with 11.12.2
  - Model Editor: double-clicking port or refport now shows that port, instead of its parent
  - Show .mga filename in the title bar
  - Fix opening mga file when its paradigm mta no longer exists
  - Attribute search: allow searching for _id=id-0065-00000001 or _guid={...}
  - Search: Autocomplete for Kind
  - Fix bug where deleting and changing archetype connpoints would fail sometimes
  - Make annotation inheritance less confusing

Release Notes of Release 13.11.14
----------------------------------
  - Binary compatibility with 11.12.2
  - Fix gme.OleIt.DumpModelGeometryXML
  - TableEditor: Excel Import/Export has been re-enabled
  - VS 2012: Register PIAs and ISIS.GME.Common in Add Reference dialog

Release Notes of Release 13.10.8
----------------------------------
  - Binary compatibility with 11.12.2
  - C# and C++ interpreter wizard support for Visual Studio 2012
  - MgaProject: don't save if opening or creating fails
  - Fix Model Editor drag-n-drop menu position
  - Fix MgaModel::get_ChildObjectByRelID for Subtypes

Release Notes of Release 13.8.28
----------------------------------
  - Binary compatibility with 11.12.2
  - XME Parser: parse XMEs with GUID connection ends, referred, set members. A future GME will write XMEs of this type
  - Fix bug with copying Folders: copied objects had duplicate GUIDs
    - GME now regenerates GUIDs for objects with duplicates when .xme and .mga files are opened. If a project includes a library twice (even indirectly), one of the libary's objects will have different GUIDs
  - XmlBackend: upgrade to SVN 1.8.1
  - Model Editor: don't draw every tab all the time. Fixes bug where the contents of multiple models were visible (META-404)
  - CSharpComponentWizard: support Visual Studio 2012
  - Disable reg-free activation for CCoreCollectionHandler (GME-418)
  - Fixed a memory leak in opening MgaMetaProjects

Release Notes of Release 13.7.10
----------------------------------
  - Binary compatibility with 11.12.2
  - Better model editor Ctrl-Mousewheel zoom
  - Fix instance/subtype FCO positions and annotations (Don't copy the archetype's registry when creating a subtype/instance)
  - During xme import to a different paradigm, don't assume that attributes must map to the only attribute in the new meta (GME-414)
  - xme import: Ignore status='meta' attributes (useful for upgrading xme to a paradigm that doesn't have the attribute)
  - Territories could cause the wrong locking under GC, so never add objects to territories
  - get_ChildObjectByRelID/get_ObjectByPath: ignore instance high bits
  - If ConstraintManager is disabled for a paradigm, it now remains disabled when the paradigm is re-registered
  - Fix refcount bug on IGMEOLEApp::MgaProject
  - Show aspect Displayed Name in Part Browser (if defined)
  - Added right-click drag-and-drop with context menu popup from Model Editor to Model Editor
  - Don't check that a component implements IGMEVersionInfo
  - Decorator SDK: Add missing PathUtil.h and Decorator.props to installer
  - Note: Visual Studio 2008 Redistributable package will not be included in the next GME release

Release Notes of Release 13.5.22
----------------------------------
  - Binary compatibility with 11.12.2
  - Fix MetaGME ExpressionChecker
  - OCL: add toString() method to Enumeration
  - Fixes for GDI (C#) decorators
  - Selected objects/connection in model editor remain selected across aspect change if still visible
  - IMgaProject::Close now fails if the save fails
  - Fix bidir conns in DsmlGenerator
  - Fix out-of-proc calls to IMga collections' GetAll
  - Register C++ interpreters as Apartment threaded (was: main STA). #define GME_COMPONENT_COM_THREAD 0 to get old behavior
  - Added General Preferences to MetaGME Folders

Release Notes of Release 13.1.24
----------------------------------
  - Fix CSharpDSMLGenerator
  - Fix IconPath
  - Fix bug where connections were never straight
  - Add Add-on event GLOBALEVENT_OPEN_PROJECT_FINISHED, to notify after project open, create or xme import
  - Fix drag-n-drop not knowing what paradigm a file is, when the xme has a different meta version than the active version

Release Notes of Release 12.12.11
----------------------------------
  - Binary compatibility with 11.12.2
  - Fix CoCreateInstance of Mga.MgaFCOs (and other collections)
  - Fix DetachFromArchetype for models containing connections
  - Fix crash when copy-and-paste fails
  - Support redirecting references with refports via drag-n-drop
  - New welcome screen
  - CSharpDSMLGenerator: handle src/dst endpoints correctly for the generated API

Release Notes of Release 12.10.18
----------------------------------
  - Binary compatibility with 11.12.2
  - Add "Reverse connection direction" to context menu for connections in the model editor
  - Fixed model editor Decorator layout bug, which could cause GME to become nonresponsive and run out of memory
  - Fix out-of-proc marshaling of GetAll for COM collections (e.g. IMgaFCOs). This allows you to run x86 Udm interpreters in 64bit GME (your CLSID will need an AppId)
  - Added progress bar to xme export
  - Fix error message if an AddOn fails during CommitTransaction, ModelOpen, or ModelClose
  - Fix bug where GME crashes with command-line .mga and Tree Browser state (GME-407)
  - Do not round decorator sizes
  - Fix some resource leaks that could result in an "Encountered an invalid argument" error
  - Better error messages when parsing xme fails

Release Notes of Release 12.8.14
----------------------------------
  - Binary compatibility with 11.12.2
  - Fixed crash when opening UML models
  - Fix bug where a derived reference with CompareToBase()==true could become an invalid ref when setting the basetype reference. This operation now throws.
  - Fix bug in AbortTransaction where AddOns would get notified of aborted changes (usually resulting in error "Meta incompatibility")
  - Fix bug in MetaInterpreter: name selection dialog should appear if Aspects linked by SameAspect have different DisplayName, but same Name
  - CSharpDSMLGenerator: output next to .mga file
  - CSharpDSMLGenerator: use interfaces instead of coclasses


Release Notes of Release 12.7.27
----------------------------------
  - Binary compatibility with 11.12.2
  - Added treeIcon and expandedTreeIcon option to Folders
  - Fix bug under CLR interpreters: get attribute fails or returns old data after default value is overridden and AbortTransaction is called
  - Fix bug where PartBrowser isn't empty after last model editor is closed (GME-405)
  - Display DispayName under "Insert New Model" (if there is only one role)
  - MetaDecorator: don't try to edit names under libraries
  - DecoratorLib: allow use of #import by clients
  - XME export: export src/dst in a predictable order for self-connections

Release Notes of Release 12.7.5
----------------------------------
  - Binary compatibility with r11.12.2
  - Critical TreeBrowser fix

Release Notes of Release 12.6.29
----------------------------------
  - Binary compatibility with r11.12.2
  - Fix MetaInterpreter bug where attributes were unordered
  - Added CSharpDsmlGenerator: generates a .NET API for MetaGME models
  - Fixed bug with deleting opened models ("Unable to refresh model")
  - Added treeIcon and expandedTreeIcon options to General Preferences.
      Icons show in the Tree Browser, and should be 16x16
  - Fix error message during xme parse
  - Fix out-of-proc activation for MgaLauncher

Release Notes of Release r12.4.20
----------------------------------
  - Binary compatibility with r11.12.2
  - Fix rare data loss in .mga files (most likely names and connection roles) (GME-395)
  - Fix truncation of real attributes in XME files (GME-391)
  - Fix compile errors when compiling GME for the first time
  - Fix preference display for undefined (meta-meta default) preferences in Object Inspector
  - Fix registry editor icons under non-standard DPI
  - Fix wrong FCO is shown in Object Inspector when selecting in Search with sorted elements (GME-392)
  - Fix crash when clicking on file: link in the Console
  - Fix JAUT.dll under gcj
  - Fix out-of-memory crashes
  - Fix: Panning Window didn't refresh when clicking on Aspect dropdown


Release Notes of Release r12.3.13
----------------------------------
  - Binary compatibility with r11.12.2
  - Show architecture of currently-running GME in the About box
  - Fix drag-to-move bug in Model Editor under x64
  - Decorators compiled against this version and later should include DecoratorLib.rc in the project (if the decorator uses any default icons)
  - Increased the maximum number of interpreters on the toolbar
  - Fix registration of .NET PIAs in Visual Studio
  - Fix IMgaModel::CreateSimpleConnDisp for Ref->Ref->Model case
  - Fix display of non-default empty value in Object Inspector
  - Fix the IErrorInfo when there's an Mga error under TRANSACTION_NON_NESTED
  - Implement Equals and GetHashCode in CSharpDsmlGenerator base class
  - Don't return E_MGA_NOT_DERIVABLE when trying to make an instance of something whose children has an instance
  - Open drag-n-dropped .html files in console

Release Notes of Release r12.2.7
----------------------------------
  - Fixed bug where elements using the default decorator had 0 size, unless an icon was specified

Release Notes of Release r12.1.24
----------------------------------
  - Fixed crash when exporting xme
  - GME_x64.msi: GME_ROOT pointed to the wrong place
  - Use PGO for increased performance


Release Notes of Release r11.12.20
----------------------------------
  - Binary compatibility with r11.12.2
  - Fixed bug where PIAs weren't included in 11.12.2
  - Multi-user bugfixes


Release Notes of Release r11.12.2
----------------------------------
  - Interpreter binary compatibility with r10.8.18
  - Backwards compatibility with .mga files from r9
  - Fixed bugs with Part Browser and Panning Window not updating sometimes
  - Memory usage improvements
  - Fixes for Object Inspector display on high DPI machines
  - Fixes for matching .mga and .xme files with their metamodel version
  - Fix FCOs not drawn at zoom <100
  - Fix some out-of-memory handling issues


Release Notes of Release r11.10.14
----------------------------------
  - Binary compatibility with r10.8.18
  - Fixed issue with copy-and-paste between GMEs
  - Fixed issue with loading scripts in the Console
  - GME.exe is now compiled with /LARGEADDRESSAWARE, which increases the maximum model size
  - Fixed model editor scroll bar for very large models
  - Performance improvements for very large models
  - Multi-user backend performance improvements, including Subversion1.7.0rc4
    - Existing Multi-user backend working copies will be faster if checked out again to upgrade the working copy version to 1.7

Release Notes of Release r11.9.20
----------------------------------
  - Fixed bug with out-of-proc GMEOLEApp use, which prevented Udm and GReAT from installing

Release Notes of Release r11.9.14
----------------------------------
  - Fixed bug with CopyFCODisp, MoveFCODisp, CopyFolderDisp, MoveFolderDisp

Release Notes of Release r11.9.9
----------------------------------
  - Binary compatibility with r10.8.18
  - Existing Raw COM interpreters will need #include "Mga.h" added to RawComponent.h
    - Alternatively, RawComponent.cpp may #import the type libraries. See a newly-generated Raw Component for an example
  - Fixed bug where .NET interpreters may cause GME.exe to run after the GUI has been closed
    - If you script GME via out-of-proc COM, closing the GME window will invalidate the IGMEOLEApp handle
  - Fixed bug with registering .NET v4.0 components
  - Added some missing enums to the GME type libraries
  - GME now allows multiple instances of the same archetype
  - Fix some SEH exceptions when scripting with Python
  - Fixed bug where the position of an FCO within an instance model would be set

Release Notes of Release r11.8.1
----------------------------------
  - Binary compatibility with r10.8.18
  - Fixed bug where AutoLayout didn't do anything

Release Notes of Release r11.7.28
----------------------------------
  - Binary compatibility with r10.8.18
  - Added .NET Primary Interop Assemblies: GME.dll, GME.MGA, GME.MGA.Meta, GME.Util, GME.MGA.Core, GME.MGA.Parser.
    - PIAs allow .NET interpreters to be strongly named, and allow interpreters to interoperate
  - Corrected some capitalization in GME COM Type Libraries.
  - Add Visual Studio 2010 project wizards
  - AutoLayout speedup
  - Minor GUI fixes
  - Memory leak and memory usage fixes
  - Other bugfixes. See escher.isis.vanderbilt.edu/JIRA/browse/GME


Release Notes of Release r11.3.23
----------------------------------
  - Binary compatibility with r10.8.18
  - Changes in the SDK requires changes to existing interpreters:
    - Change ComponentLib.idl properties to output Header File: $(InputName).h
    - For Visual Studio 2010 projects: Add $(GME_ROOT)\SDK\BON\Common\GMEInterpreter.props to both Release and Debug configurations
    - For Visual Studio 2008 projects:
      - Add $(GME_ROOT)\Interfaces and $(GME_ROOT)\GME\Interfaces to C/C++>General>Additional Include Directories
      - Add $(GME_ROOT)\bin and $(GME_ROOT)\GME\Release to MIDL>General>Additional Include Directories
  - Changes in the COM type libraries may require changes to compile existing raw interpreters and decorators:
    - Type libraries have been renamed:
      - METALib to MGAMetaLib
      - CORELib to MGACoreLib
      - UTILLib to MGAUtilLib
    - IMgaVersionInfo was renamed to IGMEVersionInfo, and moved to MGACoreLib
    - IMgaDecoratorCommon::DrawEx parameters were changed to ULONG and ULONGLONG
  - Fixed bug with disabling components in User mode when they are enabled in System mode (GME-320)
  - Fixed bug in Set Mode where unselected Atoms and Models didn't have a name label (GME-305)
  - Installer includes registry entries for more IMga COM interfaces, which fixes "No such interface" errors under Java and other non-STA code (GME-257)
  - Other bugfixes. See escher.isis.vanderbilt.edu/JIRA/browse/GME

Release Notes of Release r10.10.10
----------------------------------
  - This is a service release for r10.8.18
    Hence, it is binary compatible with that.
  - Fixing ObjectInspector bug ("could not write..." error messages)
  - Upgrade to Subversion-1.6.13/OpenSSL-0.9.8o
    compiled with VS2010. Remove GPL neon lib.
  - Fixing several memory leaks in apr/svn
  - Fixing GME-280: dont throw exceptions beyond the COM boundary
  - Remove internal XmlBackend SVN COM interfaces.
    Remove Multi User>Subversion... dialog
  - Remove SVN by CMD access method from MU backend
  - Remove ClearCase and SourceSafe support from MU backend



Release Notes of Release r10.8.18
---------------------------------
  - Fixing GME-281: Crash while closing XML progress dialog
  - Fixing race conditions in the threading logic in the
    multiuser backend
  - Fixing SAFEARRAY ownerhip and allocation strategy in the
    multiuser backend
  - Fixing multiuser progress window messages (speed locking)
  - Adding support for registering .NET components
  - Fixing GME-292: fix crash when a decorator editing operation
    causes a constraint violation


Release Notes of Release r10.6.29
---------------------------------
	- Fixing GME-287: unhandled exceptions in CCoreXmlFile:ProjectOpen.
	- Fixing GME-285: Multiuser project open silently fails
	- Inform user which automatic addons failed to start
	- Multiuser backend crash fix: comp. operator for
	  std::set did not handle NULL values properly.

Release Notes of Release r10.6.24
---------------------------------
	- This is a minor update, fixing bugs with the multiuser
	  progress window

Release Notes of Release r10.6.23
---------------------------------
  - Added autoscroll to the multiuser progress window
  - Memory leak fixes in BON
  - Fixing GME-274 and GME-256: GME hangs after clicking the scirpt button(s)
  - Fixing GME-277: automatic closing of the multiuser progress window
  - Added extra runtime checks when sending messages to the multiuser progress window
  - Removed the close (system) menu from the multiuser progress window

Release Notes of Release r10.6.4
--------------------------------
	- Rolled back breaking COM interface changes.
	  Hence, it is binary compatible with r9.8.28 and later.
	- New multiuser progress dialog (GME-271)

Release Notes of Release r10.6.1
--------------------------------
  - This release contains minor changes to the COM interface definitions.
	  Please, re-compile existing interpreters, add-ons, decorators
  - Fixing GME-270: GUI random crashes with multiuser projects
  - OBJEVENT_PRE_DESTROYED event is introduced in the MGA layer for
    easier handling of to be deleted objects
  - New crash reporting facility (using CrashRpt)
  - Fix crash with reference ports when the referee is deleted before the refport
  - Fixing GME-262: CoClasses should be named Interpreter not Decorator
  - Fixing GME-267: "One-time resolution" now reads "Remember this decision" and the logic is inverted
  - Fixing GME-266: searching for enumeration values as strings
  - Allow querying enum attributes as strings in the MGA layer (still not allowing to set these value types as strings).
  - Fixing crash when displaying connections without endpoints
  - Fixing GME-210: all empty inputs should match everything
  - Fixing GME-264: dont ask user to save file that has no modifications
  - Fixing GME-258: multiuser backend commit messages
  - Fixing GME-253: missing session folder when creating multiuser projects

Release Notes of Release r10.2.9
---------------------------------
  - This is a service release for r9.8.28
    Hence, it is binary compatible with that.
  - Fixing bugs: GME-233, GME-244

Release Notes of Release r10.2.8
---------------------------------
  - This is a service release for r9.8.28
    Hence, it is binary compatible with that.
  - Fixing bugs: GME-249, GME-250
  - Read-only attributes/preferences/properties can be
    copied (copy&paste) from the attribute panel
  - Toolbar menu fixes
  - "Show type info" meta attribute is implemented

Release Notes of Release r10.1.22
---------------------------------
  - This is a service release for r9.8.28
    Hence, it is binary compatible with that.
  - Fixed bugs: GME-246

Release Notes of Release r10.1.19
---------------------------------
  - This is a service release for r9.8.28
    Hence, it is binary compatible with that.
  - Fixed bugs: GME-217, GME-225, GME-227, GME-240, GME-241, GME-242
  - Addditional checks in the GUI for non-ASCII characters
    (we do not support them)

Release Notes of Release r10.1.11
---------------------------------
  - This is a service release for r9.8.28
    Hence, it is binary compatible with that.
  - Bugfix: crashes related inplace editing (decortator)
  - Smaller cosmetic fixes
  - Known problem: component registration does not work
    on Windows 7 (32 bit)

Release Notes of Release r10.1.4
---------------------------------
  - This is a service release for r9.8.28
    Hence, it is binary compatible with that.
  - Bugfix: crashes related to automatic unloading of
    the Constraint Manager

Release Notes of Release r9.12.29
---------------------------------
  - This is a service release for r9.8.28
    Hence, it is binary compatible with that.
  - Bugfix: random crashes while editing labels, crash (exception)
    handling

Release Notes of Release r9.12.21
---------------------------------
  - This is a service release for r9.8.28
    Hence, it is binary compatible with that.
  - It contains all features/bugfixes backported from the current
    HEAD (except COM interfaces changes)

Release Notes of Release r9.12.15
---------------------------------
  - This is a service release for r9.8.28
    Hence, it is binary compatible with that.
  - It fixes a bug which prevented the Constraint Manager
    to be disabled

Release Notes of Release r9.11.12
---------------------------------
  - This is a service release for r9.8.28 with minor bugfixes
    Hence, it is binary compatible with that.

Release Notes of Release r9.8.28
-------------------------------
  - Fixed: decorator in-place edit bug
  - Fixed: XML import freeze

Release Notes of Release r9.8.19
-------------------------------
  - GUI uses the DisplayedName property if role name and
    kind name matches
  - The GME installer automatically uninstalls previous GME
    instances
  - Removed ModelMigrate tool

Release Notes of Release r9.8.7
-------------------------------
  - User manual update

Release Notes of Release r9.7.1
-------------------------------
  - Autorouter and decorator bug fixes.
  - Visual Studio Component wizard is (back)ported for
    supporting VS2005.

Release Notes of Release r9.6.16
--------------------------------
  - Multiuser backend speed improvements (object
    deletion and movement)

Release Notes of Release r9.5.28
--------------------------------
  - Multiuser backend fixes
  - GUI speed enhancements (with AfxSetAmbientActCtx(FALSE))
  - Small bugfixes in JBON and JavaComponentRunner
  - Autorouter fixes

Release Notes of Release r9.5.20
--------------------------------
  - Small fixes in the windows installer (location of
    the UML paradigm files, GME.exe COM registration)
  - GUI manual routing bugfixes
  - ExpressionChecker (MetaGME) gets disabled during XML
    imports (for performance reasons)
  - Fixing the autoscroll problem in the Console on machines
    with IE8 and later

Release Notes of Release r9.5.8
--------------------------------
  - Subversion client library is updated to 1.6.1 (NOTE:
    working directory format is compatible with 1.6.x clients
    only)
  - Improved performance with multiuser/subversion locking
    (single transaction for all files to be "checked out")
  - Included SSH tunnel (GMEplink), default used by the multiuser
    backend (can be overidden in the standard subversion config files)
  - Improved Search plugin
  - Resizeable decorator
  - Optional manual routing for connections

Release Notes of Release r9.3.16
--------------------------------

  - JAVA_HOME environment variable to locate SDK
  - Better UAC support under Windows Vista
  - Element decorator label editing editbox flicker problem fix
  - Drawing performance fixes of new decorator infrastructure
  - Memory usage optimalization of new decorator infrastructure
  - MgaUtils became independent from .NET Framework
  - DecoratorKit update to new Decorator interface
  - NewDecoratorKit C++ example for new Decorator interface
  - BulkCommit is the default option in the multiuser backend

Release Notes of Release r9.1.23
--------------------------------
  - Fixing a misspelling in the SDK/BON/Common folder name
  - Added type library registration for the installer
  - Minor fixes in the JavaBON framework
  - Bugfixes in the GUI/decorator logic

Release Notes of Release r9.1.12
--------------------------------
  - Subversion backend library is updated to 1.5.5
  - New WiX-based installer (instead of the previous InstallShield-based)
  - xml backend crashes when creating new projects are fixed


Release Notes of Release r8.12.22
--------------------------------
  - This is an internal release, release notes are not finalized
  - The GME application and all plugins are ported to the Visual Studio 2008
    environment
  - STLport is not used anymore
  - redesigned decorator interface
  - GME view is now a standalone ActiveX control
  - xerces library is updated to 2.8
  - subversion backend library is updated to 1.4.6
  - The wizard for Raw, BON and BON2 interpreters are integrated with Visual
    Studio (this time only VS2008 is supported, the Express Edition is not)
  - the application GUI got a facelift (new MFC classes, more Vista-like look
    and feel)
  - Vista UAC is supported when the GUI/interpreters need access to protected
    resources (mostly to the registry)


Release Notes of Release r7.6.29
--------------------------------

  - Resolved bugs GME-155, GME-157: data corrupted/lost
    after trying to save the project using an invalid file path.

  - Updated UML paradigm: sync with the UDM/GReAT tools


Release Notes of Release r7.3.26
--------------------------------

  - MetaMAid add-on: sets FCOs abstract and fixes potential
    errors in specifying inheritance

  - Improved (optimized) library code: if the same library
    is included indirectly through multiple other libraries,
    GME now merges them into a single copy.

  - Navigation bar with features similar to internet browsing:
    back, forward, etc.

  - AttachLibrary and RefreshLibrary are able to process %VAR%
    style environment variables in Library names.

  - Open Model Event introduced

  - Undosize project preference. For large models it can save
    memory.

  - Numerous bugfixes including the infamous annotation
    downscale truncation/wordwrap

  - Keyboard shortcuts. A list of current shortcuts follows:

Model View:
===================================
Ctrl + 1..8 : switch between modes
Ctrl + Z, Ctrl + U : Undo/Redo
Ctrl + F : Search
Enter: show selected models (down in hier.) or follow selected references
Backspace: show parent model (up in hier.)
Ctrl + B : jump to browser (just as Locate command does)
Ctrl + I : jump to attribute panel
Z or Alt + Left : back
X or Alt + Right: forward
TAB: cycle aspect for active model
`  : cycle aspect for all open models (according to the active model's
next aspect)
C  : autoconnect element under the cursor (if mouse over object notification enabled)
Ctrl + F6: next window
Numpad +  : zoom in by 5%
Numpad -  : zomm out by 5%


Browser:
==================================
Enter : show fco for Models: it shows a model's internals
Shift + Enter: show fco in parent: shows every fco (models too) in its parent
TAB: jump to search combo box
Ctrl + TAB: cycling through Aggregate, Inheritance, Meta Pages
F2: rename active element
Ctrl + I : jump to attribute panel
Ctrl + Z, Ctrl + U : Undo/Redo
Ctrl + F : Search

Attribute Panel:
===================
TAB: cycle through elements/pages
Ctrl + B: jump to Browser

Drag & Drop for Files
==========================
1.Paradigm file (.mta, .xmp): gets registered (only if no project is open)
2.mga: project file is opened
3.xme: project file is imported




Release Notes of Release r6.11.9
---------------------------------

  - Library feature reimplementation (from scratch)

  - Namespace support in Meta and MGA libraries, in the meta interpreter
    for paradigm composition

  - Namespace Config tool added to the distribution

  - New connection end types in the GUI

  - GME Merge tool added to the distribution


Release Notes of Release r6.5.8
---------------------------------
  - Search improvements: resizebale dialog, highlights found objects in the
    browser

  - Bugfix: inconsitent titlebar texts

  - Bugfix: always enter into rename mode upon new object creation in the
    browser

  - Bugfix: All children (not just models) of folders are shown in the browser

    by double clicking it.

  - Bugfix: copying sets is fixed (cases where not all members are selected
    with the set).

Release Notes of Release r6.3.14
---------------------------------
  - Dispatch compatible method signatures introduced in IDL files

  - BON1 improvements: folder can contain other fcos than models

  - BON2 default evenlisteners react to all events (bugfix) according
    to documentation

  - Console timestamping feature

  - Parser gives better location info upon errors, exceptions

  - Toolbars are now floatable/dockable

  - Component icons (on toolbar) are programmable
    (enable/disable based on the active model)

  - Paradigm files (.xmp , .mta) if dropped on the GME window
    (while no project is opened ) will be registered (in user registry)

  - Non-sticky connection modes added to main toolbar

  - Fix for IGMEOLEApp's usage through Dispatch

  - 'View in parent' command (shortcut: Shift + Enter, or Shift + DblClick)
    introduced in ActiveBrowser to select and focus an element in its parent
    (in the editing area)

  - Fixed copying of secondary derived references pointing to also
    secondary derived objects

  - ReadOnly/ReadWrite permission flag can be applied to object hierarchies
    (accessible through the Access menu in the Browser)

  - ModelMigrate: new rules: Atom2Model, Model2Atom, Paradigm change

  - ModelMigrate: new feature: automatically generate separate scripts per rules

  - BonExtender: ordering of class declararation fixed

  - MetaInterpreter improvement: displayed name may contain special characters,
    it will be escaped

  - Bond style connection end added to METAGME paradigm

  - Content-type attribute added to MetaGME paradigm. Mime type or extension
    (identified by the leading dot) can be specified there. Appropriate editor
    will be invoked as if the user would have initiated Open or Edit action on
    such a file from Windows Explorer.

  - If GMEEditor value is defined in Mime/Database/Content Type/<mimetype>,
    GME will prefer this editor upon editing an attribute with that content type

  - Java BON bugfixes (contributed by Alex Goos)

  - Dispatch support for native OLE drag'n'drop

  - Updated Python component framework (PyGME) (VaNTH/CAPE project contrib.)
    http:


Release Notes of Release r5.11.18
---------------------------------

  - Model migration tool added to the distribution. (ModelMigrate.exe)

Release Notes of Release r5.9.11
---------------------------------
 - Updated STLport C++ library resulting performance enhancements.

 - GME is now developed and compiled with Microsoft Visual Studio.NET 2003.

 - Reliability improvements in Constraint Manager and in Expression Checker

 - Mga.dtd is no longer needed to be present in the project folder for XSL translations

 - New preference setting added for annotations: control whether to inherit them in Subtypes/Instances or not

 - Copy Smart feature: refined for better cross project copying

 - File drag and drop allowed to main GME window

 - Default Zoom level (per application) preference setting introduced

 - Port label length can be changed for models and for model references (see Miscellaneous Preferences/Port Label Length setting)

 - Active Scripting enriched with 'it' object (represents the active model). Documentation on the scirpting feature added to this manual.

 - BonExtender supports classes with up to 6 baseclasses in BON2

 - BON2 CREATED_EVENT handling improved for add-ons

 - BON2 connection methods are fixed to work properly (regarding whether reference-port or fco is connected)

 - Several JavaBON problems fixed


Release Notes of Release r4.11.10
---------------------------------

 - Metamodelling constraints are added and reviewed

 - XML backend supporting multiuser access added (see: muserdoc.txt)

 - Introduced copy closure feature

 - Java BON bug fixes

 - GME now uses STLport only (in the core layers and in interpreters)

 - OrderedSet datatype is introduced in the Constraint Manager

Release Notes of Release r4.8.25
--------------------------------

 - MetaInterpreter: Name selector dialog box for choosing the preferred name in case of equivalent objects.

 - UTF-8 encoding introduced for GME exported projects.

 - Folders may be moved, copied in GME projects.

 - Selective Closure functionality added.

 - BonExtender:	Template method getters may be generated for supporting set ordering.

 - Metainterpreter can be running in silent mode (avoids dialogs).

 - Metainterpreter: Name selector dialog box for choosing the preferred name in case of equivalent objects.

 - Metainterpreter: In case of equivalent fcos the attribute values are merged (like isAbstract, inRootFolder, ...)

 - Console window added to the GUI (accessible from GUI Automation and BON2)

 - Dual interfaces added to the GUI Automation along with C++ wrappers in BON2

 - AutoRouter & Model Grid take care of labels (default turned off in preferences)

 - MGA generates and maintaines GUIDs for projects

 - Introduced version and metaversion project properties (MGA, XML, BON2)

 - Enhanced project properties dialog

 - Attribute Browser displays read-write project properties when RootFolder is selected

 - MetaGME: got rid of ParadigmSheet attributes (project properties are used instead)

 - MgaMetaParser does not generate checksum as GUID, if one is found in the paradigm file

 - Custom versioning for projects and paradigms is introduced (eg.: version column in paradigm dialogs)

 - The old MetaInterpreter became deprecated, hence removed from the installation

Release Notes of Release r4.5.18
--------------------------------

- AutoLayout feature is introduced as a plug-in component

- XSLT based project migration support is added

- A new Panning Window assists the user to navigate in huge models

- The XML parser components of GME are now using Xerces 2.4.0.
  The redistributed Xerces dll is changed. If you are using
  xerces in your project(s), please upgrade to the same version

- The Table Editor now has export/import to/from Excel.
  As it has always been with the table editor, only the name
  and attributes of an FCO should be edited.


Bug fixes since r4.3.17:

- Fixed: Attribute panel deterministically made wrong assignments (#GME-52)

- XOR operator is fixed in the Constraint Manager (#GME-63)

- Attribute browser properly updates values on focus changes (#GME-64)

- Fixed: Stale objects in Constraint Manager (#GME-47)

- Recursive contraint function problems are fixed (#GME-48, #GME-51)

- Crash problems on attribute changes are fixed (#GME-61)

- BONExtender generated classes now have public static member variables (#GME-41)

- HFSM animator problem is fixed (#GME-55)

- ConfigureComponent.exe does not delete (accidentally) the existing .dsp file (#GME-50)

- ConfigureComponent now properly sets the event flag for generated Add-ons (#GME-49)

- Special symbols in attribute names are filtered by the Meta Interpreter (#GME-45)

- The "Save Project" dialog is not presented if the project does not contain changes (#GME-40)


Release Notes of Release r4.3.17
--------------------------------
- Java BON framework is added

- External text editor support for multiline attributes

- Periodic autosave feature added (configurable through
  the GME settings dialog)

- Enhanced printing and print preview

- Enhanced logging (under the <USER PROFILE>/Application Data/GME folder)

- Several MetaInterpreter and BonExtender enhancements

- Application specific notifications can be sent through the MGA layer.
  The XML parser does signal the beginning and the completion of the import process



Bug fixes since r4.3.2:

- Copy & paste between different instances of GME now
  properly handles references, subtypes and sets (BUG #20)

- Paste to text editors (eg: notepad) does work again (BUG #15)

- BON2 is compatibile with Visual C 7.1 (BUG #23)

- Major BON2 object factory problems are solved (BUG #16)

Release Notes of Release r4.2.3
--------------------------------
This release contains the following significant improvements
over the previous public version:

- New and redesigned  Builder Object Network (BON2)

- New and enhanced MetaInterpreter along with skeleton code generator
for BON2 and a metainterpreter framework

- The GUI supports OLE Automation

Bug fixes since r4.1.8:

- Redirecting references with draging selfconnected objects
problem is fixed

- Libraries in the meta environment are now supported by the
new metainterpreter

- References pointing to root objects are now properly handled in the GUI

- Type/subtype information is properly displayed in the attribute browser

- Object visualization support is added to the GUI Automation interface

- Enhanced logging


Release Notes of Release r4.1.8 (internal)
-------------------------------
Bug fixes:

- Problems with enumaration attributes in the attribute editor are
fixed.

- 'Ever growing MGA file' problem is fixed

Release Notes of Release r3.12.18 (internal)
---------------------------------

- A new and enhanced MetaInterpreter along with a code generator for BON
(BONExtender) included

- A new metainterpreter framework included

- The GUI now supports OLE Automation. See the type library in GME.exe for
further reference.

- Tutorials are updated to reflect the changes in the metainterpreter

- Run-time logging added (text files are created for all GME sessions)

Bug fixes:

- Problems with running GME as normal user are fixed

- Bug fixes in the new BON implementation


Release Notes of Release r3.11.14 (internal)
---------------------------------

Bug fixes:

- Component Toolbar positioning bug is fixed

- Attribute browser editing bug ("Object Inspector could not write attribute data")) is fixed

- Editor state is preserved after OBJEVENT_ATTR service

- OBJEVENT_CLOSEMODEL is delivered at project close

- Several TableEditor enhancements

- Minor improvements in the standard decorator

- Canonical XML dump (entities are now ordered in the XME files)

Release Notes of Release r3.10.13
---------------------------------

- New Table Editor plug-in is introduced: to use it, open
File/Register Components, select the GME Table Editor, and press
Toggle. Afterwards, it can be launched from the component toolbar
or the File menu/run Plugins command.

- Exported model xml files are now using the .xme filename
extension, however, the file format did not change.

- New default decorator is included providing nicer visualization.
Type/instance visualization is enhanced and configurable
through model preferences. The old decorator is still available in
the release.

- Object and connection autorouter preference settings are now
available from the context menus.

- The beta release of a new Builder Object Network (BON2) and
the Meta Object Network (MON) is included in the release. No
documentation is available as of now.

- Performance enhancements in the GUI code.

- Dispatch based add-ons are supported.

- CreateNewComponent utility enhancements.

Bug fixes:

- Several problems fixed in the attribute panel.

- Minor fixes and extensions in the Constraint Manager.

- Repeated undo key (CTRL-Z) problem is solved

- Component tooltip problems and toolbar positioning troubles are fixed

- Circular references are now properly handled by the GUI code

- GME Emergency Events after drag-ctrl-drop are eliminated

- Browser updates itself after deleting non-visible objects (in non-expanded models)

- Browser "Preserve Tree State" feature is working

- CR/LF problems in exported and (re)imported models are fixed



Release Notes of Release r3.4.29
--------------------------------
This is a bugfix release. Copy operations which contain connections
are no longer crashing the GME application. Other small fixes:
- GME does not update the system registry now, so users without
administrative privileges will not encounter warning messages
- Decorkit COM macros are updated
- Small visualization problems (type/instance) are fixed in the
standard decorator

Release Notes of GME 3
--------------------------------
This release contains lot of improvements and changes over the
previous versions. Therefore it is even more important to read
carefully the first section (about the upgrade process).

- We have a new OCL Constraint Manager with lot of nice features and
 comprehensive documentation (as part of the User Manual)

- GME3 uses the Windows Installer (2.0) interface which provides a
standard mechanism to redistribute system DLLs.

- We got rid of the "2000" prefix everywhere. If you cannot open/import
your previous meta models (created with MetaGME2000) choose the MetaGME
paradigm which is compatible with MetaGME2000.

- The User Manual is updated.

- Types and Instances are indicated in the model editor.

- BON is restructured, so please update your interpreter sources.

- Undo problems fixed and its performance is enhanced.

- This release also contains or pattern language interpreter

Minor fixes/enhancements:

- GUI improvements (new icons, consistent style)

- XML based (inter-process) clipboard support in the Browser

- Library handling problems in the MGA

- Single click, or multiple selections in the Browser now
  update the contents of the Attribute Panel.

- Context menus are enabled in GME connection modes

- Hotkeys are assigned to switch between GME modes (CTRL+1 - CTRL+6)

Release Notes of Release r11.21.1 follow.

- Interpreter invocation from the Browser

- Connection hotspot feature is now controllable
  (and is disabled for inheritance icons in the meta environment)

- The installer now removes old paradigms and components
  from the windows registry. Please, re-register your paradigms
  from the .xmp files and recompile your interpreters

- Libraries can be renamed in the browser

- Interpreters based on BON can be compiled using Visual Studio.Net

Release Notes of Release r9.20.1 follow.
----------------------------------------

This is a release containing two redesigned components.

The new Model Browser provides almost the same functionality
as the previous one, but its stability was significantly
improved and the new internal architecture enables us to
enhance this component more easily in the future.

The Attribute Browser is now implemented as a standalone
ActiveX component. It provides a unified interface for
attributes, preferences and properties.

In our previous service release a new component has been added.
It is a plug-in, i.e a paradigm independent component.
It provides comprehensive searching capability.


Release Notes of Release r12.18.1 follow.
-----------------------------------------

New features:

- Model libraries allow the creation and reuse of model
repositories. Any project can serve as a library and can be
attached to other projects. Models in the library can be
subtyped and instantiated in the dependent projects. Any
subsequent changes in the library will propagate to the
projects that use it. This feature required file format
modifications. GME will automatically convert binary
files to the new format (saving the original with the .orig
extension), but it is a relatively slow process. However,
subsequent accesses to the file are as fast as before.

- ODBC backend. In addition to the MS Repository and binary
file backends, now ODBC is also supported.

- Annotations, i.e. textual labels, can be added to models.
Their visibility in different aspects, as well as color,
size, typeface, etc, can be controlled from a dialog box.

- Visual connection preference specification. Autorouter
preferences can now be specified on a per connection basis.
In connection mode, the object the cursor moves over is
highlighted. When the cursor is near one side of the object,
a connection point appears. If it is clicked, the new
connection will stick to the selected side of the object.
This selection can later be overwritten using the registry
editor.

- Constraint manager improvements. The constraint manager has
been reengineered. There are new functions to access
information about connections. There is a dialog box that
gives context information for debugging constraints.

- Extensive tutorial. There is a new extensive end-to-end
tutorial included with this version.


As always, please, export all your projects in XML format using
your current version. Please, uninstall any previous version of
GME before installing the program on your machine.
You will need to parse the paradigm defintion files (*.xmp)
that you are using (File menu New Project command and Add
from File button). After this you will be able to load the
projects by importing the xml files your created with the old
version.

Also, some of the COM interfaces changed. So, recompile all of
your existing interpreters. If you are using BON, then you should
use the CretaNewComponent.exe tool in an empty directory and port
you own interpreter files, so that they comply with the interface
changes.

Note that most of the files in the release are read only. So, for
example, if you try to reregister the meta paradigm using the
xml version of the definition, not the binary (.mta), then it'll
fail, since it will try to overwrite the MetaModeling2000.mta
file. If you want to play with the metamodeling paradigm like
that, make a copy of the directory and change the read only
attributes.

Known issues:

- Every time you have a new version of a paradigm, you can try
to load existing projects in binary (.mga) format. The program
will ask you whether to upgrade to the new paradigm. However,
the upgrade process may fail. If so, export you projects in
xml format with the old paradigm, reregister the new paradigm
version, and import the xml file. The xml import/export utility
is the more robust way model migration is supported. Note that
if you make a paradigm change that invalidates existing models,
the xml import operation will also fail. We are working on a
general solution to the model migration problem.


The previous version (v1.2) added these new features:

- User-defined drawing capability. How GME displays model objects is
now decided by external components called decorators. The previous
appearence of boxes for models and icons for other objects is preserved
as the default visualization (also implemented by decorators included with
the GME release). However, users can write their own decorators.
The only requirement is that decorators have to implement a COM interface that
GME uses when it needs to display the objects. The new UML class diagram
paradigm sample comes with its own decorator that displays classnames,
stereotypes and attributes inside the class icon and resizes it accordingly.
The GME metamodeling paradigm has a similar decorator as well. A
decorator shell is also provided with this release to help you write
your own decorators. Note that connection visualization has not changed
and is not customizable.

- Modeless dockable dialog for attributes and preferences. The attributes
and preferences dialogs have been merged into one tabbed dialog window that
is always visible and dockable to the main window frame. These dialogs also
display the object name. There are multiple ways to select the object whose
attributes and preferences are shown. All the context menus (even from the
browser now) provide access like before. If a new object is inserted,
pasted or dropped, its attributes and preferences will be immediately shown.
Finally, simply selecting an object by clicking on it, has the same effect.
Note that currently the attributes and preferences dialog does not support
multiple object selection.

- Add-on and plug-in support. The add-on mechanism has been updated and
tested. Whenever a data file is loaded, the activated add-ons are also
loaded automatically. Add-ons listen to events; the event set listened to
is specified through the component configurator GUI (ComponentConfig.exe) .
For efficiency reasons, Addons cannot  be Builder Object Network components.
Plug-ins are noew accessible throufg a separate command in the File menu.

- OCL syntax checker add-on for the metamodeling environment. As a sample
add-on, the metamodeling environment now comes with this nice helper
tool. Every time a constraint expression attribute is changed this add-on
is activated. Note that the target paradigm information is not available
to this tool, therefore, it cannot check arguments and parameters, such as
kindname. These can only be checked at constraint evaluation time in your
target environment.

- Toolbar button/interpreter association capability. Interpreters and
plug-ins can now register toolbar icons. An icon is either a resource in
the component itself, or a separate icon file. If a project is loaded, the
 registered toolbar icons of all the active components are displayed in
the toolbar, providing a user friendly way to start components.  The icon
information is stored in the registry under the 'Icon' field in the
components registry node. Its format is either [<modulename>],
<resource key>, or a full pathname of an image file.

- Component interface version checking. Starting with version 1.2, GME
will be very conservative about component interface versions. First, the
components that make up GME must always be present and have identical
component interface version numbers. Components are also expected to be
built against the very same interace as the GME executing them, although
only a warning is displayed when starting incompatible components. There
is no way to change the interface versions of compiled binaries. The only
possible way to update the inteface version number is to recompile the
components against the up-to-date interface files.

- Type inheritance refinements. The previous restriction, that only root
models can be derived from or instantiated, has been relaxed. Now a
model type can be derived or instantiated provided none of its ancestors
or descendants (in the containment hierarchy) have any subtypes or instances.

- Paste Special commands. Objects on the clipboard can now be pasted as
references, subtypes or intances through these commands available through
the regular menu and the context menus. References can also be redirected
using the Redirection Paste command in the context menu. The usual
restrictions still apply, i.e. paradigm violations and other illegal
operations are not allowed. Note that the paste special commands only
work if the source of the clipboard data is the same project open in the
same GME instance.

- Instant connections through context menus. In the regular edit mode
connections can now be made by the Connect command in the context menu.
Selecting this command changes the cursor to the connect cursor. A connection
will be made to the object that is left clicked next. (Or by selecting the
Connect command on the destination object as well.) Note that any other
operation, such as mode change, window change, new object creation, cancels
the connection operation.

- GME icons settings now allow two macros,$PARADIGMDIR and $PROJECTDIR, that
resolve to the directory of the current paradigm definition or project file,
respectively.

- Updated high-level C++ interpreter interface (Builder Object Network or BON
for ahort). BON now uses the IMgaComponentEx COM interface (also new in this
version). The biggest change is that the Invoke function has been replaced by
InvokeEx, which clearly separates the focus object from the selected objects.
(Depending on the invocation method both of these parameters may be empty.)
Components using the old BON will still work, however, upon invocation a
warning is message displayed reminding users to upgrade the component code
to fully comply with the new BON.

- Tutorials. Two short, simple tutorials have been prepared on metamodeling
and metamodel composition.

- A sample UML class diagram drawing paradigm. To illustrate the user-defined
drawing capabilities of this version, we are including this simple
paradigm as an example. Note that no interpreter is included.

- Java high-level interpreter interface (alpha release). We have prepared
this interface that is very similar to the high-level C++ interface, the
Builder Object Network (BON). This is an experimental version, not tested
thoroughly. A fairly severe restriction is that it is based on Visual J++,
because we have used the Java/COM bridge from Microsoft.

************************************************
2. System Requirements
************************************************

GME is supported on Windows XP, Vista, 7, 8, and 10.
GME does not work on Windows Server Core.

************************************************
3. Installation
************************************************

You must uninstall GME versions prior to version
9.1.12 using the Control Panel before installing GME.

************************************************
4. Directories
************************************************

The GME root directory contains a copy of this document. The
bin subdirectory has the GME executable and all necessary
dlls. The doc directory contains the User's Manual in pdf
format. The Paradigms/MetaGME directory contains the metamodeling
environment. The Paradigms directory contains example paradigms,
metamodels, models, and a demo and runtime system for the SF
paradigm. The SDK directory contains the BON for interpreter
developers, the DecoratorLib for decorator writers and the
Java interpreter interface.

************************************************
5. Contact info
************************************************

Send comments, questions to akos.ledeczi@vanderbilt.edu
Send bug reports to gme-supp@isis.vanderbilt.edu or escher.isis.vanderbilt.edu/JIRA/browse/GME

************************************************
6. License
************************************************
GME contains software covered by other licenses and copyrights.

Antlr (PCCTS)
  Copyright (c) 1989-1998 Parr Research Corporation with Purdue University and
  	AHPCRC, University of Minnesota

Xerces
  Copyright (c) 1999-2004 The Apache Software Foundation.

TokenEx
  Copyright (c) 1999  Daniel Madden <daniel.madden@compaq.com>

TreeCtrlEx
  Copyright (c) 1997-2001 Bendik Engebretsen <bendik@techsoft.no>

Regexp
  Copyright (c) 1986 by University of Toronto. (Henry Spencer)

ColourPicker, ColourPopup
  Copyright (c) 1998  Chris Maunder <chrismaunder@codeguru.com> and
  	Alexander Bischofberger <bischofb@informatik.tu-muenchen.de>

MFC Grid Control - inplace editing class (CInPlaceEdit)
  Copyright (c) 1998-2002 Chris Maunder <cmaunder@mail.com>. All Rights Reserved.

zlib
  Copyright (C) 1995-1998 Jean-loup Gailly and Mark Adler

CrashRpt
  Copyright (c) 2003, Michael Carruth

Subversion
  Copyright (c) 2000-2009 CollabNet.

APR (Apache Portable Runtime)
  Copyright (c) 1999-2004 The Apache Software Foundation.

OpenSSL
  Copyright (c) 1998-2008 The OpenSSL Project.

Serf
  Copyright 2002-2004 Justin Erenkrantz and Greg Stein

neon HTTP/WebDAV client library
  Copyright (c)  2001-2008 Joe Orton


*****************************************************
Appendix A. The Apache Software License, Version 1.1
*****************************************************

  Copyright (c) 1999-2004 The Apache Software Foundation.  All rights
  reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions
  are met:

  1. Redistributions of source code must retain the above copyright
     notice, this list of conditions and the following disclaimer.

  2. Redistributions in binary form must reproduce the above copyright
     notice, this list of conditions and the following disclaimer in
     the documentation and/or other materials provided with the
     distribution.

  3. The end-user documentation included with the redistribution,
     if any, must include the following acknowledgment:
        "This product includes software developed by the
         Apache Software Foundation (http:
     Alternately, this acknowledgment may appear in the software itself,
     if and wherever such third-party acknowledgments normally appear.

  4. The names "Xalan" and "Apache Software Foundation" must
     not be used to endorse or promote products derived from this
     software without prior written permission. For written
     permission, please contact apache@apache.org.

  5. Products derived from this software may not be called "Apache",
     nor may "Apache" appear in their name, without prior written
     permission of the Apache Software Foundation.

  THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESSED OR IMPLIED
  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
  OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  DISCLAIMED.  IN NO EVENT SHALL THE APACHE SOFTWARE FOUNDATION OR
  ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
  USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
  OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
  SUCH DAMAGE.
  ====================================================================

  This software consists of voluntary contributions made by many
  individuals on behalf of the Apache Software Foundation and was
  originally based on software copyright (c) 1999, International
  Business Machines, Inc., www.ibm.com.  For more
  information on the Apache Software Foundation, please see
  <www.apache.org/>.
