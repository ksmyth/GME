
#ifndef MGA_MGAREGISTRAR_H
#define MGA_MGAREGISTRAR_H

#include "CommonVersionInfo.h"
// --------------------------- CMgaRegistrar

class ATL_NO_VTABLE CMgaRegistrar : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CMgaRegistrar, &__uuidof(MgaRegistrar)>,
	public IDispatchImpl<IMgaRegistrar2, &__uuidof(IMgaRegistrar2), &__uuidof(__MGAUtilLib)>,
	public ISupportErrorInfoImpl2<&__uuidof(IMgaRegistrar), &__uuidof(IMgaRegistrar2)>,
	public IGMEVersionInfoImpl
{
public:
	CMgaRegistrar() { }

DECLARE_REGISTRY_RESOURCEID(IDR_MGAREGISTRAR)
DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CMgaRegistrar)
	COM_INTERFACE_ENTRY(IMgaRegistrar)
	COM_INTERFACE_ENTRY(IMgaRegistrar2)
	COM_INTERFACE_ENTRY2(IDispatch, IMgaRegistrar2)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
	COM_INTERFACE_ENTRY_IID(__uuidof(IGMEVersionInfo), IGMEVersionInfoImpl)
END_COM_MAP()

// ------- Methods

public:
	STDMETHOD(get_IconPath)(regaccessmode_enum mode, BSTR *path);
	STDMETHOD(put_IconPath)(regaccessmode_enum mode, BSTR path);
	STDMETHOD(GetIconPathDisp)(regaccessmode_enum mode, BSTR *path) { return get_IconPath( mode, path); }
	STDMETHOD(SetIconPathDisp)(regaccessmode_enum mode, BSTR path) { return put_IconPath( mode, path); }
	STDMETHOD(get_ShowMultipleView)(regaccessmode_enum mode, VARIANT_BOOL *enabled);
	STDMETHOD(put_ShowMultipleView)(regaccessmode_enum mode, VARIANT_BOOL enabled);
	STDMETHOD(GetShowMultipleViewDisp)(regaccessmode_enum mode, VARIANT_BOOL *enabled) { return get_ShowMultipleView( mode, enabled); }
	STDMETHOD(SetShowMultipleViewDisp)(regaccessmode_enum mode, VARIANT_BOOL enabled) { return put_ShowMultipleView( mode, enabled); }
	STDMETHOD(get_EventLoggingEnabled)(regaccessmode_enum mode, VARIANT_BOOL *enabled);
	STDMETHOD(put_EventLoggingEnabled)(regaccessmode_enum mode, VARIANT_BOOL enabled);
	STDMETHOD(GetEventLoggingEnabledDisp)(regaccessmode_enum mode, VARIANT_BOOL *enabled) { return get_EventLoggingEnabled( mode, enabled); }
	STDMETHOD(SetEventLoggingEnabledDisp)(regaccessmode_enum mode, VARIANT_BOOL enabled) { return put_EventLoggingEnabled( mode, enabled); }
	STDMETHOD(get_AutosaveEnabled)(regaccessmode_enum mode, VARIANT_BOOL *enabled);
	STDMETHOD(put_AutosaveEnabled)(regaccessmode_enum mode, VARIANT_BOOL enabled);
	STDMETHOD(GetAutosaveEnabledDisp)(regaccessmode_enum mode, VARIANT_BOOL *enabled) { return get_AutosaveEnabled( mode, enabled); }
	STDMETHOD(SetAutosaveEnabledDisp)(regaccessmode_enum mode, VARIANT_BOOL enabled) { return put_AutosaveEnabled( mode, enabled); }
	STDMETHOD(get_AutosaveFreq)(regaccessmode_enum mode, long *secs);
	STDMETHOD(put_AutosaveFreq)(regaccessmode_enum mode, long secs);
	STDMETHOD(GetAutosaveFreqDisp)(regaccessmode_enum mode, long *secs) { return get_AutosaveFreq( mode, secs); }
	STDMETHOD(SetAutosaveFreqDisp)(regaccessmode_enum mode, long secs) { return put_AutosaveFreq( mode, secs); }
	STDMETHOD(get_AutosaveUseDir)(regaccessmode_enum mode, VARIANT_BOOL *use);
	STDMETHOD(put_AutosaveUseDir)(regaccessmode_enum mode, VARIANT_BOOL use);
	STDMETHOD(GetAutosaveUseDirDisp)(regaccessmode_enum mode, VARIANT_BOOL *use) { return get_AutosaveUseDir( mode, use); }
	STDMETHOD(SetAutosaveUseDirDisp)(regaccessmode_enum mode, VARIANT_BOOL use) { return put_AutosaveUseDir( mode, use); }
	STDMETHOD(get_AutosaveDir)(regaccessmode_enum mode, BSTR *dir);
	STDMETHOD(put_AutosaveDir)(regaccessmode_enum mode, BSTR dir);
	STDMETHOD(GetAutosaveDirDisp)(regaccessmode_enum mode, BSTR *dir) { return get_AutosaveDir( mode, dir); }
	STDMETHOD(SetAutosaveDirDisp)(regaccessmode_enum mode, BSTR dir) { return put_AutosaveDir( mode, dir); }
	STDMETHOD(get_ExternalEditorEnabled)(regaccessmode_enum mode, VARIANT_BOOL *enabled);
	STDMETHOD(put_ExternalEditorEnabled)(regaccessmode_enum mode, VARIANT_BOOL enabled);
	STDMETHOD(GetExternalEditorEnabledDisp)(regaccessmode_enum mode, VARIANT_BOOL *enabled) { return get_ExternalEditorEnabled( mode, enabled); }
	STDMETHOD(SetExternalEditorEnabledDisp)(regaccessmode_enum mode, VARIANT_BOOL enabled) { return put_ExternalEditorEnabled( mode, enabled); }
	STDMETHOD(get_ExternalEditor)(regaccessmode_enum mode, BSTR *path);
	STDMETHOD(put_ExternalEditor)(regaccessmode_enum mode, BSTR path);
	STDMETHOD(GetExternalEditorDisp)(regaccessmode_enum mode, BSTR *path) { return get_ExternalEditor( mode, path); }
	STDMETHOD(SetExternalEditorDisp)(regaccessmode_enum mode, BSTR path) { return put_ExternalEditor( mode, path); }
	STDMETHOD(get_UseAutoRouting)(regaccessmode_enum mode, VARIANT_BOOL *enabled);
	STDMETHOD(put_UseAutoRouting)(regaccessmode_enum mode, VARIANT_BOOL enabled);
	STDMETHOD(GetUseAutoRoutingDisp)(regaccessmode_enum mode, VARIANT_BOOL *enabled) { return get_UseAutoRouting( mode, enabled); }
	STDMETHOD(SetUseAutoRoutingDisp)(regaccessmode_enum mode, VARIANT_BOOL enabled) { return put_UseAutoRouting( mode, enabled); }
	STDMETHOD(get_LabelAvoidance)(regaccessmode_enum mode, VARIANT_BOOL *enabled);
	STDMETHOD(put_LabelAvoidance)(regaccessmode_enum mode, VARIANT_BOOL enabled);
	STDMETHOD(GetLabelAvoidanceDisp)(regaccessmode_enum mode, VARIANT_BOOL *enabled) { return get_LabelAvoidance( mode, enabled); }
	STDMETHOD(SetLabelAvoidanceDisp)(regaccessmode_enum mode, VARIANT_BOOL enabled) { return put_LabelAvoidance( mode, enabled); }
	STDMETHOD(get_ScriptEngine)(regaccessmode_enum mode, BSTR *path);
	STDMETHOD(put_ScriptEngine)(regaccessmode_enum mode, BSTR path);
	STDMETHOD(GetScriptEngineDisp)(regaccessmode_enum mode, BSTR *path) { return get_ScriptEngine( mode, path); }
	STDMETHOD(SetScriptEngineDisp)(regaccessmode_enum mode, BSTR path) { return put_ScriptEngine( mode, path); }
	STDMETHOD(GetDefZoomLevel)(regaccessmode_enum mode, BSTR *zlevel);
	STDMETHOD(SetDefZoomLevel)(regaccessmode_enum mode, BSTR zlevel);
	STDMETHOD(GetMouseOverNotify)(regaccessmode_enum mode, VARIANT_BOOL *enabled);
	STDMETHOD(SetMouseOverNotify)(regaccessmode_enum mode, VARIANT_BOOL enabled);
	STDMETHOD(GetRealNmbFmtStr)(regaccessmode_enum mode, BSTR *fmtStr);
	STDMETHOD(SetRealNmbFmtStr)(regaccessmode_enum mode, BSTR fmtStr);
	STDMETHOD(GetTimeStamping)(regaccessmode_enum mode, VARIANT_BOOL *enabled);
	STDMETHOD(SetTimeStamping)(regaccessmode_enum mode, VARIANT_BOOL enabled);
	STDMETHOD(GetNavigation)(regaccessmode_enum mode, VARIANT_BOOL *enabled);
	STDMETHOD(SetNavigation)(regaccessmode_enum mode, VARIANT_BOOL enabled);
	STDMETHOD(GetUndoQueueSize)(regaccessmode_enum mode, BSTR *queuesz);
	STDMETHOD(SetUndoQueueSize)(regaccessmode_enum mode, BSTR queuesz);
	STDMETHOD(get_EdgeSmoothMode)(regaccessmode_enum mode, edgesmoothmode_enum* smoothMode);
	STDMETHOD(put_EdgeSmoothMode)(regaccessmode_enum mode, edgesmoothmode_enum smoothMode);
	STDMETHOD(GetEdgeSmoothModeDisp)(regaccessmode_enum mode, edgesmoothmode_enum* smoothMode) { return get_EdgeSmoothMode( mode, smoothMode); }
	STDMETHOD(SetEdgeSmoothModeDisp)(regaccessmode_enum mode, edgesmoothmode_enum smoothMode) { return put_EdgeSmoothMode( mode, smoothMode); }
	STDMETHOD(get_FontSmoothMode)(regaccessmode_enum mode, fontsmoothmode_enum* smoothMode);
	STDMETHOD(put_FontSmoothMode)(regaccessmode_enum mode, fontsmoothmode_enum smoothMode);
	STDMETHOD(GetFontSmoothModeDisp)(regaccessmode_enum mode, fontsmoothmode_enum* smoothMode) { return get_FontSmoothMode( mode, smoothMode); }
	STDMETHOD(SetFontSmoothModeDisp)(regaccessmode_enum mode, fontsmoothmode_enum smoothMode) { return put_FontSmoothMode( mode, smoothMode); }

	STDMETHOD(get_Paradigms)(regaccessmode_enum mode, VARIANT *names);
	STDMETHOD(GetParadigmsDisp)(regaccessmode_enum mode, VARIANT *names) { return get_Paradigms( mode, names); }
	STDMETHOD(RegisterParadigmFromData)(BSTR connstr, BSTR *newname, regaccessmode_enum mode);
	STDMETHOD(RegisterParadigmFromDataDisp)(BSTR connstr, regaccessmode_enum mode, BSTR *newname);
	STDMETHOD(RegisterParadigm)(BSTR name, BSTR connstr, BSTR version, VARIANT guid, regaccessmode_enum mode);
	STDMETHOD(QueryParadigmAllGUIDs)(BSTR name, VARIANT *names, regaccessmode_enum mode);
	STDMETHOD(QueryParadigm)(BSTR name, BSTR *connstr, VARIANT *quid, regaccessmode_enum mode);
	STDMETHOD(get_ParadigmGUIDString)(regaccessmode_enum mode, BSTR name, BSTR *guidstr);
	STDMETHOD(GetParadigmGUIDStringDisp)(regaccessmode_enum mode, BSTR name, BSTR *guidstr) { return get_ParadigmGUIDString( mode, name, guidstr); }
	STDMETHOD(UnregisterParadigmGUID)(BSTR name, VARIANT v, regaccessmode_enum mode);
	STDMETHOD(UnregisterParadigm)(BSTR name, regaccessmode_enum mode);
	STDMETHOD(VersionFromGUID)(BSTR name, VARIANT guid, BSTR *ver, regaccessmode_enum mode);
	STDMETHOD(GUIDFromVersion)(BSTR name, BSTR ver, VARIANT* guid, regaccessmode_enum mode);

	STDMETHOD(get_Components)(regaccessmode_enum mode, VARIANT *progids);
	STDMETHOD(GetComponentsDisp)(regaccessmode_enum mode, VARIANT *progids) { return get_Components( mode, progids); }
	STDMETHOD(RegisterComponent)(BSTR progid, componenttype_enum type, BSTR desc, regaccessmode_enum mode);
	STDMETHOD(QueryComponent)(BSTR progid, componenttype_enum *type, BSTR *desc, regaccessmode_enum mode);
	STDMETHOD(UnregisterComponent)(BSTR progid, regaccessmode_enum mode);
	STDMETHOD(put_ComponentExtraInfo)(regaccessmode_enum mode, 
										BSTR progid, BSTR name, BSTR newVal);
	STDMETHOD(get_ComponentExtraInfo)(regaccessmode_enum mode, 
										BSTR progid, BSTR name, BSTR* pVal);
	STDMETHOD(SetComponentExtraInfoDisp)(regaccessmode_enum mode, 
		BSTR progid, BSTR name, BSTR newVal) { return put_ComponentExtraInfo( mode, progid, name, newVal); }
	STDMETHOD(GetComponentExtraInfoDisp)(regaccessmode_enum mode, 
		BSTR progid, BSTR name, BSTR* pVal) { return get_ComponentExtraInfo( mode, progid, name, pVal); }
	STDMETHOD(get_LocalDllPath)(BSTR progid, BSTR* pVal);
	STDMETHOD(GetLocalDllPathDisp)(BSTR progid, BSTR* pVal) { return get_LocalDllPath( progid, pVal); }

	HRESULT LocalDllPath(BSTR progid,BSTR *pVal);

	STDMETHOD(get_AssociatedComponents)(BSTR paradigm, componenttype_enum type, regaccessmode_enum mode, VARIANT *progids);
	STDMETHOD(GetAssociatedComponentsDisp)(BSTR paradigm, componenttype_enum type, regaccessmode_enum mode, VARIANT *progids) { return get_AssociatedComponents( paradigm, type, mode, progids); }
	STDMETHOD(get_AssociatedParadigms)(BSTR progid, regaccessmode_enum mode, VARIANT *paradigms);
	STDMETHOD(GetAssociatedParadigmsDisp)(BSTR progid, regaccessmode_enum mode, VARIANT *paradigms) { return get_AssociatedParadigms( progid, mode, paradigms); }
	STDMETHOD(Associate)(BSTR progid, BSTR paradigm, regaccessmode_enum mode);
	STDMETHOD(Disassociate)(BSTR progid, BSTR paradigm, regaccessmode_enum mode);
	STDMETHOD(IsAssociated)(BSTR progid, BSTR paradigm, VARIANT_BOOL *is_ass, VARIANT_BOOL *can_ass, regaccessmode_enum mode);

// --- Actions

	STDMETHOD(RegisterComponentLibrary)(BSTR path, regaccessmode_enum mode);
	STDMETHOD(UnregisterComponentLibrary)(BSTR path, regaccessmode_enum mode);

	// --- IMgaRegistrar2

	STDMETHOD(put_ParadigmExtraInfo)(regaccessmode_enum mode, BSTR paradigmName, BSTR paradigmVersionGuid, BSTR name, BSTR newVal);
	STDMETHOD(get_ParadigmExtraInfo)(regaccessmode_enum mode, BSTR paradigmName, BSTR paradigmVersionGuid, BSTR name, BSTR* pVal);
	STDMETHOD(SetParadigmExtraInfoDisp)(BSTR paradigmName, BSTR paradigmVersionGuid, BSTR name, BSTR newVal, regaccessmode_enum mode = REGACCESS_SYSTEM) { return this->put_ParadigmExtraInfo(mode, paradigmName, paradigmVersionGuid, name, newVal); }
	STDMETHOD(GetParadigmExtraInfoDisp)(BSTR paradigmName, BSTR paradigmVersionGuid, BSTR name, BSTR* pVal) { return this->get_ParadigmExtraInfo(REGACCESS_BOTH, paradigmName, paradigmVersionGuid, name, pVal); }
};

#endif//MGA_MGAREGISTRAR_H
