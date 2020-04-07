#ifndef _GUIMETA_INCLUDED_
#define _GUIMETA_INCLUDED_

#include "stdafx.h"
#include "GMEStd.h"
#include "DynMenu.h"

// "defined in" hierarchy not captured here! (we create meta objs using it though)
// --> cannot be traversed standalone

class CGuiMetaProject
{
public:
	CGuiMetaProject(CComPtr<IMgaMetaProject> &mgaPt);
	~CGuiMetaProject();
public:
	CString name;
	CString guid;
	CString displayedName;
	CGuiMetaFolder *rootFolder;
	CGuiMetaBaseTable metaTable;
	CGuiMetaBaseList metaRemainingList;
	CComPtr<IMgaMetaProject> mgaMeta;
	int minMenuCmdID;
	int maxMenuCmdID;

	static CGuiMetaProject *theInstance;
public:
	void RegisterMetaObject(metaref_type id, CGuiMetaBase *mobj);
	int GetNewMenuCmdID()		{ ASSERT(maxMenuCmdID < DECORATOR_CTX_MENU_MINID - 1); return maxMenuCmdID++; };
	bool CmdIDInRange(int id);
	GMECommandCode CmdType(int id,CString &cmdName);
	bool FindCommand(int cmdId,CString &label);
	CGuiMetaBase *GetGuiMeta(metaref_type key);
	CGuiMetaAttribute *GetGuiMetaAttribute(metaref_type key);
	CGuiMetaFco *GetGuiMetaFco(metaref_type key);
	CGuiMetaModel *GetGuiMetaModel(metaref_type key);
};

class CGuiMetaBase
{
public:
	CGuiMetaBase() {}
	virtual ~CGuiMetaBase() {}
public:
	metaref_type metaref;
	CString name;
	CString displayedName;
	CComPtr<IMgaMetaBase> mgaMeta;
public:
	void Init(CComPtr<IMgaMetaBase> &mgaPt);		// init members + put in table
	void CreateFcos(CComPtr<IMgaMetaFCOs> &mmFcos);
};

class CGuiMetaFolder : public CGuiMetaBase
{
public:
	CGuiMetaFolder(CComPtr<IMgaMetaFolder> &mgaPt);
	virtual ~CGuiMetaFolder() {}
public:
public:
};

class CGuiMetaAttribute : public CGuiMetaBase
{
public:
	CGuiMetaAttribute(CComPtr<IMgaMetaAttribute> &mgaPt,CGuiMetaFco *o);
public:
	CGuiMetaFco *owner;
	CString prompt;
public:
};

class CGuiMetaFieldAttribute : public CGuiMetaAttribute
{
public:
	CGuiMetaFieldAttribute(CComPtr<IMgaMetaAttribute> &mgaPt,CGuiMetaFco *o);
	virtual ~CGuiMetaFieldAttribute() {}
public:
public:
};

class CGuiMetaMenuAttribute : public CGuiMetaAttribute
{
public:
	CGuiMetaMenuAttribute(CComPtr<IMgaMetaAttribute> &mgaPt,CGuiMetaFco *o);
	virtual ~CGuiMetaMenuAttribute() { }
public:
public:
};

class CGuiMetaToggleAttribute : public CGuiMetaAttribute
{
public:
	CGuiMetaToggleAttribute(CComPtr<IMgaMetaAttribute> &mgaPt,CGuiMetaFco *o);
	virtual ~CGuiMetaToggleAttribute() {}
public:
public:
};

class CGuiMetaFco : public CGuiMetaBase
{
public:
	CGuiMetaFco(CComPtr<IMgaMetaFCO> &mgaPt);
	CGuiMetaFco(CComPtr<IMgaMetaModel> &mgaPt);
	virtual ~CGuiMetaFco() {}
public:
	CGuiMetaAttributeList attrs;
public:
	void SetAttributes();
};

class CGuiMetaModel : public CGuiMetaFco
{
public:
	CGuiMetaModel(CComPtr<IMgaMetaModel> &mgaPt);
	virtual ~CGuiMetaModel();
public:
	CGuiMetaAspectList aspects;
	bool partsSet;
private:
	CGuiObjectList parts;			// for part browser
public:
	CGuiObjectList &GetParts();
	void DeleteParts();
	void ResetParts();
	void InitParts();
	void InitAspectBox(CComboBox *box);
	void InitAspectList(CListBox *list);
	CGuiMetaAspect *GetFirstAspect();
	CGuiMetaAspect *FindAspect(CString nm);
	CGuiMetaAspect *FindAspect(int ind);
	int NumberOfAspects();
	int MapAspect(CComPtr<IMgaMetaRole> &role,int index);
	bool GetPartByRole(CComPtr<IMgaMetaRole> &theRole,CComPtr<IMgaMetaAspect> &theAspect,CComPtr<IMgaMetaPart> &thePart);
};

class CGuiMetaAspect : public CGuiMetaBase
{
public:
	CGuiMetaAspect(CComPtr<IMgaMetaAspect>& mgaPt, CGuiMetaModel* o, int ind);
	virtual ~CGuiMetaAspect();
public:
	CGuiMetaModel *owner;
	CDynMenu *insertModelMenu;
	CDynMenu *insertAtomMenu;
	CDynMenu *insertReferenceMenu;
	CDynMenu *insertSetMenu;
	int index;
	CGuiMetaAttributeList attrs;
public:
	void GetMetaAspect(CComPtr<IMgaMetaAspect> &mAspect);
	void InitContextMenu(CMenu* menu, const CString& label, CDynMenu* dm);
	void InitContextMenu(CMenu* menu);
	void ResetContextMenu(CMenu* menu);
	bool FindCommand(int cmdId, CString &label);
	bool GetRoleByName(const CString& roleName, CComPtr<IMgaMetaRole>& theRole, bool dispName = false);
	bool GetPartByRole(CComPtr<IMgaMetaRole>& theRole, CComPtr<IMgaMetaPart>& thePart);
	bool IsLinkedRole(CComPtr<IMgaMetaRole>& theRole);
	bool IsPrimary(CComPtr<IMgaFCO> fco);
	bool IsPrimaryByRoleName(CComPtr<IMgaFCO> fco);
	bool IsPrimaryByRoleName(CComPtr<IMgaMetaRole> role);
	bool CheckFcosBeforeInsertion(CComPtr<IMgaFCOs> fcos);
};

#endif // whole file