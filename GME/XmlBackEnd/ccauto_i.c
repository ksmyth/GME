/* this file contains the actual definitions of */
/* the IIDs and CLSIDs */

/* link this file in with the server and any clients */


/* File created by MIDL compiler version 5.01.0164 */
/* at Mon Sep 27 12:18:12 2004
 */
/* Compiler settings for Y:\Projects\GME\GMESRC\GME\XmlBackEnd\ccauto.IDL:
    Oicf (OptLev=i2), W1, Zp8, env=Win32, ms_ext, c_ext
    error checks: allocation ref bounds_check enum stub_data 
*/
//@@MIDL_FILE_HEADING(  )
#ifdef __cplusplus
extern "C"{
#endif 


#ifndef __IID_DEFINED__
#define __IID_DEFINED__

typedef struct _IID
{
    unsigned long x;
    unsigned short s1;
    unsigned short s2;
    unsigned char  c[8];
} IID;

#endif // __IID_DEFINED__

#ifndef CLSID_DEFINED
#define CLSID_DEFINED
typedef IID CLSID;
#endif // CLSID_DEFINED

const IID LIBID_ClearCase = {0xB22C7EFA,0x5A5E,0x11D3,{0xB1,0xCD,0x00,0xC0,0x4F,0x8E,0xCE,0x2F}};


const IID IID_IClearCase = {0xB22C7EC6,0x5A5E,0x11D3,{0xB1,0xCD,0x00,0xC0,0x4F,0x8E,0xCE,0x2F}};


const IID IID_ICCVOBObject = {0xB22C7EC9,0x5A5E,0x11D3,{0xB1,0xCD,0x00,0xC0,0x4F,0x8E,0xCE,0x2F}};


const IID IID_ICCActivity = {0xB22C7EF9,0x5A5E,0x11D3,{0xB1,0xCD,0x00,0xC0,0x4F,0x8E,0xCE,0x2F}};


const IID IID_ICCAttribute = {0xB22C7EED,0x5A5E,0x11D3,{0xB1,0xCD,0x00,0xC0,0x4F,0x8E,0xCE,0x2F}};


const IID IID_ICCAttributeType = {0xB22C7EEB,0x5A5E,0x11D3,{0xB1,0xCD,0x00,0xC0,0x4F,0x8E,0xCE,0x2F}};


const IID IID_ICCLock = {0xB22C7EF7,0x5A5E,0x11D3,{0xB1,0xCD,0x00,0xC0,0x4F,0x8E,0xCE,0x2F}};


const IID IID_ICCHistoryRecord = {0xB22C7ECF,0x5A5E,0x11D3,{0xB1,0xCD,0x00,0xC0,0x4F,0x8E,0xCE,0x2F}};


const IID IID_ICCVOB = {0xB22C7ECB,0x5A5E,0x11D3,{0xB1,0xCD,0x00,0xC0,0x4F,0x8E,0xCE,0x2F}};


const IID IID_ICCAttributeTypes = {0xB22C7EEC,0x5A5E,0x11D3,{0xB1,0xCD,0x00,0xC0,0x4F,0x8E,0xCE,0x2F}};


const IID IID_ICCBranchType = {0xB22C7EE7,0x5A5E,0x11D3,{0xB1,0xCD,0x00,0xC0,0x4F,0x8E,0xCE,0x2F}};


const IID IID_ICCFile = {0xB22C7ED1,0x5A5E,0x11D3,{0xB1,0xCD,0x00,0xC0,0x4F,0x8E,0xCE,0x2F}};


const IID IID_ICCVersion = {0xB22C7ED6,0x5A5E,0x11D3,{0xB1,0xCD,0x00,0xC0,0x4F,0x8E,0xCE,0x2F}};


const IID IID_ICCView = {0xB22C7ECD,0x5A5E,0x11D3,{0xB1,0xCD,0x00,0xC0,0x4F,0x8E,0xCE,0x2F}};


const IID IID_ICCUCMObject = {0xB22C7F31,0x5A5E,0x11D3,{0xB1,0xCD,0x00,0xC0,0x4F,0x8E,0xCE,0x2F}};


const IID IID_ICCStream = {0xB22C7F2D,0x5A5E,0x11D3,{0xB1,0xCD,0x00,0xC0,0x4F,0x8E,0xCE,0x2F}};


const IID IID_ICCProjectVOB = {0xB22C7F27,0x5A5E,0x11D3,{0xB1,0xCD,0x00,0xC0,0x4F,0x8E,0xCE,0x2F}};


const IID IID_ICCBaseline = {0xB22C7F2C,0x5A5E,0x11D3,{0xB1,0xCD,0x00,0xC0,0x4F,0x8E,0xCE,0x2F}};


const IID IID_ICCActivities = {0xB22C7F30,0x5A5E,0x11D3,{0xB1,0xCD,0x00,0xC0,0x4F,0x8E,0xCE,0x2F}};


const IID IID_ICCComponent = {0xB22C7F2B,0x5A5E,0x11D3,{0xB1,0xCD,0x00,0xC0,0x4F,0x8E,0xCE,0x2F}};


const IID IID_ICCElement = {0xB22C7ED2,0x5A5E,0x11D3,{0xB1,0xCD,0x00,0xC0,0x4F,0x8E,0xCE,0x2F}};


const IID IID_ICCCheckedOutFile = {0xB22C7ED9,0x5A5E,0x11D3,{0xB1,0xCD,0x00,0xC0,0x4F,0x8E,0xCE,0x2F}};


const IID IID_ICCTrigger = {0xB22C7EF5,0x5A5E,0x11D3,{0xB1,0xCD,0x00,0xC0,0x4F,0x8E,0xCE,0x2F}};


const IID IID_ICCTriggerType = {0xB22C7EF3,0x5A5E,0x11D3,{0xB1,0xCD,0x00,0xC0,0x4F,0x8E,0xCE,0x2F}};


const IID IID_ICCTriggerTypeBuilder = {0xB22C7EF2,0x5A5E,0x11D3,{0xB1,0xCD,0x00,0xC0,0x4F,0x8E,0xCE,0x2F}};


const IID IID_ICCHyperlinkType = {0xB22C7EDF,0x5A5E,0x11D3,{0xB1,0xCD,0x00,0xC0,0x4F,0x8E,0xCE,0x2F}};


const IID IID_ICCLabelType = {0xB22C7EE3,0x5A5E,0x11D3,{0xB1,0xCD,0x00,0xC0,0x4F,0x8E,0xCE,0x2F}};


const IID IID_ICCTriggers = {0xB22C7EF6,0x5A5E,0x11D3,{0xB1,0xCD,0x00,0xC0,0x4F,0x8E,0xCE,0x2F}};


const IID IID_ICCStreams = {0xB22C7F45,0x5A5E,0x11D3,{0xB1,0xCD,0x00,0xC0,0x4F,0x8E,0xCE,0x2F}};


const IID IID_ICCComponents = {0xB22C7F43,0x5A5E,0x11D3,{0xB1,0xCD,0x00,0xC0,0x4F,0x8E,0xCE,0x2F}};


const IID IID_ICCFolder = {0xB22C7F2A,0x5A5E,0x11D3,{0xB1,0xCD,0x00,0xC0,0x4F,0x8E,0xCE,0x2F}};


const IID IID_ICCProjects = {0xB22C7F41,0x5A5E,0x11D3,{0xB1,0xCD,0x00,0xC0,0x4F,0x8E,0xCE,0x2F}};


const IID IID_ICCProject = {0xB22C7F29,0x5A5E,0x11D3,{0xB1,0xCD,0x00,0xC0,0x4F,0x8E,0xCE,0x2F}};


const IID IID_ICCProjectPolicy = {0xB22C7F28,0x5A5E,0x11D3,{0xB1,0xCD,0x00,0xC0,0x4F,0x8E,0xCE,0x2F}};


const IID IID_ICCBaselines = {0xB22C7F44,0x5A5E,0x11D3,{0xB1,0xCD,0x00,0xC0,0x4F,0x8E,0xCE,0x2F}};


const IID IID_ICCFolders = {0xB22C7F42,0x5A5E,0x11D3,{0xB1,0xCD,0x00,0xC0,0x4F,0x8E,0xCE,0x2F}};


const IID IID_ICCViews = {0xB22C7ECE,0x5A5E,0x11D3,{0xB1,0xCD,0x00,0xC0,0x4F,0x8E,0xCE,0x2F}};


const IID IID_ICCBranch = {0xB22C7EE9,0x5A5E,0x11D3,{0xB1,0xCD,0x00,0xC0,0x4F,0x8E,0xCE,0x2F}};


const IID IID_ICCVersions = {0xB22C7ED7,0x5A5E,0x11D3,{0xB1,0xCD,0x00,0xC0,0x4F,0x8E,0xCE,0x2F}};


const IID IID_ICCLabel = {0xB22C7EE5,0x5A5E,0x11D3,{0xB1,0xCD,0x00,0xC0,0x4F,0x8E,0xCE,0x2F}};


const IID IID_ICCLabels = {0xB22C7EE6,0x5A5E,0x11D3,{0xB1,0xCD,0x00,0xC0,0x4F,0x8E,0xCE,0x2F}};


const IID IID_ICCBranches = {0xB22C7EEA,0x5A5E,0x11D3,{0xB1,0xCD,0x00,0xC0,0x4F,0x8E,0xCE,0x2F}};


const IID IID_ICCBranchTypes = {0xB22C7EE8,0x5A5E,0x11D3,{0xB1,0xCD,0x00,0xC0,0x4F,0x8E,0xCE,0x2F}};


const IID IID_ICCHyperlink = {0xB22C7EE1,0x5A5E,0x11D3,{0xB1,0xCD,0x00,0xC0,0x4F,0x8E,0xCE,0x2F}};


const IID IID_ICCHyperlinkTypes = {0xB22C7EE0,0x5A5E,0x11D3,{0xB1,0xCD,0x00,0xC0,0x4F,0x8E,0xCE,0x2F}};


const IID IID_ICCLabelTypes = {0xB22C7EE4,0x5A5E,0x11D3,{0xB1,0xCD,0x00,0xC0,0x4F,0x8E,0xCE,0x2F}};


const IID IID_ICCLocks = {0xB22C7EF8,0x5A5E,0x11D3,{0xB1,0xCD,0x00,0xC0,0x4F,0x8E,0xCE,0x2F}};


const IID IID_ICCTriggerTypes = {0xB22C7EF4,0x5A5E,0x11D3,{0xB1,0xCD,0x00,0xC0,0x4F,0x8E,0xCE,0x2F}};


const IID IID_ICCAttributes = {0xB22C7EEE,0x5A5E,0x11D3,{0xB1,0xCD,0x00,0xC0,0x4F,0x8E,0xCE,0x2F}};


const IID IID_ICCHistoryRecords = {0xB22C7ED0,0x5A5E,0x11D3,{0xB1,0xCD,0x00,0xC0,0x4F,0x8E,0xCE,0x2F}};


const IID IID_ICCHyperlinks = {0xB22C7EE2,0x5A5E,0x11D3,{0xB1,0xCD,0x00,0xC0,0x4F,0x8E,0xCE,0x2F}};


const IID IID_ICCCheckedOutFiles = {0xB22C7EDA,0x5A5E,0x11D3,{0xB1,0xCD,0x00,0xC0,0x4F,0x8E,0xCE,0x2F}};


const IID IID_ICCCheckedOutFileQuery = {0xB22C7EDC,0x5A5E,0x11D3,{0xB1,0xCD,0x00,0xC0,0x4F,0x8E,0xCE,0x2F}};


const IID IID_ICCElements = {0xB22C7ED3,0x5A5E,0x11D3,{0xB1,0xCD,0x00,0xC0,0x4F,0x8E,0xCE,0x2F}};


const IID IID_ICCVOBs = {0xB22C7ECC,0x5A5E,0x11D3,{0xB1,0xCD,0x00,0xC0,0x4F,0x8E,0xCE,0x2F}};


const IID IID_ICCBaselineComparison = {0xB22C7F2E,0x5A5E,0x11D3,{0xB1,0xCD,0x00,0xC0,0x4F,0x8E,0xCE,0x2F}};


const IID IID_ICCProjectVOBs = {0xB22C7F40,0x5A5E,0x11D3,{0xB1,0xCD,0x00,0xC0,0x4F,0x8E,0xCE,0x2F}};


const IID IID_IClearTool = {0xB22C7EC7,0x5A5E,0x11D3,{0xB1,0xCD,0x00,0xC0,0x4F,0x8E,0xCE,0x2F}};


const CLSID CLSID_Application = {0xB22C7EFB,0x5A5E,0x11D3,{0xB1,0xCD,0x00,0xC0,0x4F,0x8E,0xCE,0x2F}};


const CLSID CLSID_ClearTool = {0xB22C7EFC,0x5A5E,0x11D3,{0xB1,0xCD,0x00,0xC0,0x4F,0x8E,0xCE,0x2F}};


const CLSID CLSID_CCVOB = {0xB22C7EFD,0x5A5E,0x11D3,{0xB1,0xCD,0x00,0xC0,0x4F,0x8E,0xCE,0x2F}};


const CLSID CLSID_CCVOBs = {0xB22C7EFE,0x5A5E,0x11D3,{0xB1,0xCD,0x00,0xC0,0x4F,0x8E,0xCE,0x2F}};


const CLSID CLSID_CCView = {0xB22C7EFF,0x5A5E,0x11D3,{0xB1,0xCD,0x00,0xC0,0x4F,0x8E,0xCE,0x2F}};


const CLSID CLSID_CCViews = {0xB22C7F00,0x5A5E,0x11D3,{0xB1,0xCD,0x00,0xC0,0x4F,0x8E,0xCE,0x2F}};


const CLSID CLSID_CCHistoryRecord = {0xB22C7F01,0x5A5E,0x11D3,{0xB1,0xCD,0x00,0xC0,0x4F,0x8E,0xCE,0x2F}};


const CLSID CLSID_CCHistoryRecords = {0xB22C7F02,0x5A5E,0x11D3,{0xB1,0xCD,0x00,0xC0,0x4F,0x8E,0xCE,0x2F}};


const CLSID CLSID_CCVOBObject = {0xB22C7F03,0x5A5E,0x11D3,{0xB1,0xCD,0x00,0xC0,0x4F,0x8E,0xCE,0x2F}};


const CLSID CLSID_CCFile = {0xB22C7F04,0x5A5E,0x11D3,{0xB1,0xCD,0x00,0xC0,0x4F,0x8E,0xCE,0x2F}};


const CLSID CLSID_CCElement = {0xB22C7F05,0x5A5E,0x11D3,{0xB1,0xCD,0x00,0xC0,0x4F,0x8E,0xCE,0x2F}};


const CLSID CLSID_CCElements = {0xB22C7F06,0x5A5E,0x11D3,{0xB1,0xCD,0x00,0xC0,0x4F,0x8E,0xCE,0x2F}};


const CLSID CLSID_CCVersion = {0xB22C7F07,0x5A5E,0x11D3,{0xB1,0xCD,0x00,0xC0,0x4F,0x8E,0xCE,0x2F}};


const CLSID CLSID_CCVersions = {0xB22C7F08,0x5A5E,0x11D3,{0xB1,0xCD,0x00,0xC0,0x4F,0x8E,0xCE,0x2F}};


const CLSID CLSID_CCCheckedOutFile = {0xB22C7F09,0x5A5E,0x11D3,{0xB1,0xCD,0x00,0xC0,0x4F,0x8E,0xCE,0x2F}};


const CLSID CLSID_CCCheckedOutFiles = {0xB22C7F0A,0x5A5E,0x11D3,{0xB1,0xCD,0x00,0xC0,0x4F,0x8E,0xCE,0x2F}};


const CLSID CLSID_CCCheckedOutFileQuery = {0xB22C7F0B,0x5A5E,0x11D3,{0xB1,0xCD,0x00,0xC0,0x4F,0x8E,0xCE,0x2F}};


const CLSID CLSID_CCHyperlink = {0xB22C7F0C,0x5A5E,0x11D3,{0xB1,0xCD,0x00,0xC0,0x4F,0x8E,0xCE,0x2F}};


const CLSID CLSID_CCHyperlinks = {0xB22C7F0D,0x5A5E,0x11D3,{0xB1,0xCD,0x00,0xC0,0x4F,0x8E,0xCE,0x2F}};


const CLSID CLSID_CCHyperlinkType = {0xB22C7F0E,0x5A5E,0x11D3,{0xB1,0xCD,0x00,0xC0,0x4F,0x8E,0xCE,0x2F}};


const CLSID CLSID_CCHyperlinkTypes = {0xB22C7F0F,0x5A5E,0x11D3,{0xB1,0xCD,0x00,0xC0,0x4F,0x8E,0xCE,0x2F}};


const CLSID CLSID_CCLabel = {0xB22C7F10,0x5A5E,0x11D3,{0xB1,0xCD,0x00,0xC0,0x4F,0x8E,0xCE,0x2F}};


const CLSID CLSID_CCLabels = {0xB22C7F11,0x5A5E,0x11D3,{0xB1,0xCD,0x00,0xC0,0x4F,0x8E,0xCE,0x2F}};


const CLSID CLSID_CCLabelType = {0xB22C7F12,0x5A5E,0x11D3,{0xB1,0xCD,0x00,0xC0,0x4F,0x8E,0xCE,0x2F}};


const CLSID CLSID_CCLabelTypes = {0xB22C7F13,0x5A5E,0x11D3,{0xB1,0xCD,0x00,0xC0,0x4F,0x8E,0xCE,0x2F}};


const CLSID CLSID_CCBranch = {0xB22C7F14,0x5A5E,0x11D3,{0xB1,0xCD,0x00,0xC0,0x4F,0x8E,0xCE,0x2F}};


const CLSID CLSID_CCBranches = {0xB22C7F15,0x5A5E,0x11D3,{0xB1,0xCD,0x00,0xC0,0x4F,0x8E,0xCE,0x2F}};


const CLSID CLSID_CCBranchType = {0xB22C7F16,0x5A5E,0x11D3,{0xB1,0xCD,0x00,0xC0,0x4F,0x8E,0xCE,0x2F}};


const CLSID CLSID_CCBranchTypes = {0xB22C7F17,0x5A5E,0x11D3,{0xB1,0xCD,0x00,0xC0,0x4F,0x8E,0xCE,0x2F}};


const CLSID CLSID_CCAttribute = {0xB22C7F18,0x5A5E,0x11D3,{0xB1,0xCD,0x00,0xC0,0x4F,0x8E,0xCE,0x2F}};


const CLSID CLSID_CCAttributes = {0xB22C7F19,0x5A5E,0x11D3,{0xB1,0xCD,0x00,0xC0,0x4F,0x8E,0xCE,0x2F}};


const CLSID CLSID_CCAttributeType = {0xB22C7F1A,0x5A5E,0x11D3,{0xB1,0xCD,0x00,0xC0,0x4F,0x8E,0xCE,0x2F}};


const CLSID CLSID_CCAttributeTypes = {0xB22C7F1B,0x5A5E,0x11D3,{0xB1,0xCD,0x00,0xC0,0x4F,0x8E,0xCE,0x2F}};


const CLSID CLSID_CCTrigger = {0xB22C7F1C,0x5A5E,0x11D3,{0xB1,0xCD,0x00,0xC0,0x4F,0x8E,0xCE,0x2F}};


const CLSID CLSID_CCTriggers = {0xB22C7F1D,0x5A5E,0x11D3,{0xB1,0xCD,0x00,0xC0,0x4F,0x8E,0xCE,0x2F}};


const CLSID CLSID_CCTriggerType = {0xB22C7F1E,0x5A5E,0x11D3,{0xB1,0xCD,0x00,0xC0,0x4F,0x8E,0xCE,0x2F}};


const CLSID CLSID_CCTriggerTypes = {0xB22C7F1F,0x5A5E,0x11D3,{0xB1,0xCD,0x00,0xC0,0x4F,0x8E,0xCE,0x2F}};


const CLSID CLSID_CCTriggerTypeBuilder = {0xB22C7F20,0x5A5E,0x11D3,{0xB1,0xCD,0x00,0xC0,0x4F,0x8E,0xCE,0x2F}};


const CLSID CLSID_CCLock = {0xB22C7F21,0x5A5E,0x11D3,{0xB1,0xCD,0x00,0xC0,0x4F,0x8E,0xCE,0x2F}};


const CLSID CLSID_CCLocks = {0xB22C7F22,0x5A5E,0x11D3,{0xB1,0xCD,0x00,0xC0,0x4F,0x8E,0xCE,0x2F}};


const CLSID CLSID_CCActivity = {0xB22C7F23,0x5A5E,0x11D3,{0xB1,0xCD,0x00,0xC0,0x4F,0x8E,0xCE,0x2F}};


const CLSID CLSID_CCActivities = {0xB22C7F47,0x5A5E,0x11D3,{0xB1,0xCD,0x00,0xC0,0x4F,0x8E,0xCE,0x2F}};


const CLSID CLSID_CCProjectVOB = {0xB22C7F32,0x5A5E,0x11D3,{0xB1,0xCD,0x00,0xC0,0x4F,0x8E,0xCE,0x2F}};


const CLSID CLSID_CCProjectVOBs = {0xB22C7F33,0x5A5E,0x11D3,{0xB1,0xCD,0x00,0xC0,0x4F,0x8E,0xCE,0x2F}};


const CLSID CLSID_CCUCMObject = {0xB22C7F48,0x5A5E,0x11D3,{0xB1,0xCD,0x00,0xC0,0x4F,0x8E,0xCE,0x2F}};


const CLSID CLSID_CCProject = {0xB22C7F34,0x5A5E,0x11D3,{0xB1,0xCD,0x00,0xC0,0x4F,0x8E,0xCE,0x2F}};


const CLSID CLSID_CCProjects = {0xB22C7F35,0x5A5E,0x11D3,{0xB1,0xCD,0x00,0xC0,0x4F,0x8E,0xCE,0x2F}};


const CLSID CLSID_CCFolder = {0xB22C7F37,0x5A5E,0x11D3,{0xB1,0xCD,0x00,0xC0,0x4F,0x8E,0xCE,0x2F}};


const CLSID CLSID_CCFolders = {0xB22C7F38,0x5A5E,0x11D3,{0xB1,0xCD,0x00,0xC0,0x4F,0x8E,0xCE,0x2F}};


const CLSID CLSID_CCComponent = {0xB22C7F39,0x5A5E,0x11D3,{0xB1,0xCD,0x00,0xC0,0x4F,0x8E,0xCE,0x2F}};


const CLSID CLSID_CCComponents = {0xB22C7F3A,0x5A5E,0x11D3,{0xB1,0xCD,0x00,0xC0,0x4F,0x8E,0xCE,0x2F}};


const CLSID CLSID_CCBaseline = {0xB22C7F3B,0x5A5E,0x11D3,{0xB1,0xCD,0x00,0xC0,0x4F,0x8E,0xCE,0x2F}};


const CLSID CLSID_CCBaselines = {0xB22C7F3C,0x5A5E,0x11D3,{0xB1,0xCD,0x00,0xC0,0x4F,0x8E,0xCE,0x2F}};


const CLSID CLSID_CCStream = {0xB22C7F3D,0x5A5E,0x11D3,{0xB1,0xCD,0x00,0xC0,0x4F,0x8E,0xCE,0x2F}};


const CLSID CLSID_CCStreams = {0xB22C7F3E,0x5A5E,0x11D3,{0xB1,0xCD,0x00,0xC0,0x4F,0x8E,0xCE,0x2F}};


const CLSID CLSID_CCBaselineComparison = {0xB22C7F3F,0x5A5E,0x11D3,{0xB1,0xCD,0x00,0xC0,0x4F,0x8E,0xCE,0x2F}};


const CLSID CLSID_CCProjectPolicy = {0xB22C7F46,0x5A5E,0x11D3,{0xB1,0xCD,0x00,0xC0,0x4F,0x8E,0xCE,0x2F}};


#ifdef __cplusplus
}
#endif

