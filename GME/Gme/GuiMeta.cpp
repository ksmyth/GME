#include "stdafx.h"
#include "GMEStd.h"

#include "meta.h"
#include "GuiMeta.h"
#include "GuiObject.h"

CComBSTR StringFromGUID2(const CComVariant& guid);

// helper functions

static bool FindMenu(CMenu* menu, const CString& menuName, CMenu*& subMenu)
{
	subMenu = 0;
	for(unsigned int i = 0; i < menu->GetMenuItemCount(); i++) {
		CString label;
		menu->GetMenuString(i, label, MF_BYPOSITION);
		if (label == menuName) {
			subMenu = menu->GetSubMenu(i);
			return true;
		}
	}
	return false;
}

static bool FindMenuItem(CMenu* menu, const CString& itemName, unsigned int& pos)
{
	if (menu) {
		CString label;
		for(pos = 0; pos < menu->GetMenuItemCount(); pos++) {
			CString label;
			menu->GetMenuString(pos, label, MF_BYPOSITION);
			if (label == itemName)
				return true;
		}
	}
	return false;
}

static bool FindMenuItem(CMenu* menu, const CString& menuName, CMenu*& subMenu, const CString& itemName, unsigned int& pos)
{
	FindMenu(menu, menuName, subMenu);
	return FindMenuItem(subMenu, itemName, pos);
}

// assumes that all constructors are called already inside a transaction

////////////////////////////// CGuiMetaProject /////////////////////////////

CGuiMetaProject *CGuiMetaProject::theInstance = 0;

CGuiMetaProject::CGuiMetaProject(CComPtr<IMgaMetaProject> &mgaPt)
{
	ASSERT(theInstance == 0);
	theInstance = this;
	minMenuCmdID = maxMenuCmdID = GME_DYN_MENU_MINID;
	metaTable.InitHashTable(97);
	mgaMeta = mgaPt;
	try {
		{
			CComBSTR bstr;
			COMTHROW(mgaMeta->get_Name(&bstr));
			CopyTo(bstr,name);
		}
		{
			CComBSTR bstr;
			COMTHROW(mgaMeta->get_DisplayedName(&bstr));
			CopyTo(bstr,displayedName);
		}
		{
			CComVariant pguid;
			COMTHROW(mgaMeta->get_GUID(&pguid));
			guid = StringFromGUID2(pguid);
		}

		CComPtr<IMgaMetaFolder> mmFolder;
		COMTHROW(mgaMeta->get_RootFolder(&mmFolder));
		new CGuiMetaFolder(mmFolder);
	}
	catch(hresult_exception &) {
	}
}

CGuiMetaProject::~CGuiMetaProject()
{
	POSITION pos = metaTable.GetStartPosition();
	void *key;
	CGuiMetaBase *guiMeta;
	while(pos) {
		metaTable.GetNextAssoc(pos,key,guiMeta);
		delete guiMeta;
	}

	pos =  metaRemainingList.GetHeadPosition();
	while (pos) 
		delete metaRemainingList.GetNext(pos);

	theInstance = 0;
}

bool CGuiMetaProject::CmdIDInRange(int id)
{
	return (id >= minMenuCmdID && id < maxMenuCmdID);
}

GMECommandCode CGuiMetaProject::CmdType(int id,CString &/*cmdName*/)
{
	ASSERT(CmdIDInRange(id));
	return GME_CMD_CONTEXT;
}

CGuiMetaBase *CGuiMetaProject::GetGuiMeta(metaref_type key)
{
	CGuiMetaBase *base;
	metaTable.Lookup((void *)key,base);
	return base;
}

void CGuiMetaProject::RegisterMetaObject(metaref_type key, CGuiMetaBase *mobj)
{
	CGuiMetaBase *base;
	if (metaTable.Lookup((void*) key, base))
		metaRemainingList.AddTail(mobj);
	else
		metaTable[(void *)key] = mobj;
}


CGuiMetaAttribute *CGuiMetaProject::GetGuiMetaAttribute(metaref_type key)
{
	return dynamic_cast<CGuiMetaAttribute *>(GetGuiMeta(key));
}

CGuiMetaFco *CGuiMetaProject::GetGuiMetaFco(metaref_type key)
{
	return dynamic_cast<CGuiMetaFco *>(GetGuiMeta(key));
}

CGuiMetaModel *CGuiMetaProject::GetGuiMetaModel(metaref_type key)
{
	return dynamic_cast<CGuiMetaModel *>(GetGuiMeta(key));
}

////////////////////////////// CGuiMetaBase /////////////////////////////

void CGuiMetaBase::Init(CComPtr<IMgaMetaBase> &mgaPt)
{
	mgaMeta = mgaPt;
	try {
			{
				CComBSTR bstr;
				COMTHROW(mgaMeta->get_Name(&bstr));
				CopyTo(bstr,name);
			}
			{
				CComBSTR bstr;
				COMTHROW(mgaMeta->get_DisplayedName(&bstr));
				CopyTo(bstr,displayedName);
				if(displayedName.IsEmpty())
					displayedName = name;
			}
			COMTHROW(mgaMeta->get_MetaRef(&metaref));
	}
	catch(hresult_exception &) {
	}
	CGuiMetaProject::theInstance->RegisterMetaObject(metaref, this);
}


void CGuiMetaBase::CreateFcos(CComPtr<IMgaMetaFCOs> &mmFcos)
{
	CComPtr<IMgaMetaFCO> mmFco;
	MGACOLL_ITERATE(IMgaMetaFCO,mmFcos) {
		CComPtr<IMgaMetaModel> mmModel;
		mmFco = MGACOLL_ITER;
		HRESULT hr;
		hr = mmFco.QueryInterface(&mmModel);
		if(FAILED(hr))
			new CGuiMetaFco(mmFco);
		else
			new CGuiMetaModel(mmModel);
	}
	MGACOLL_ITERATE_END;
}

////////////////////////////// CGuiMetaFolder /////////////////////////////

CGuiMetaFolder::CGuiMetaFolder(CComPtr<IMgaMetaFolder> &mgaPt)
{
	CComPtr<IMgaMetaBase> basePt;
	try {
		mgaPt.QueryInterface(&basePt);
		Init(basePt);
		CComPtr<IMgaMetaFolders> mmFolders;
		CComPtr<IMgaMetaFolder> mmFolder;
		COMTHROW(mgaPt->get_DefinedFolders(&mmFolders));
		MGACOLL_ITERATE(IMgaMetaFolder,mmFolders) {
			mmFolder = MGACOLL_ITER;
			new CGuiMetaFolder(mmFolder);
		}
		MGACOLL_ITERATE_END;

		CComPtr<IMgaMetaFCOs> mmFcos;
		COMTHROW(mgaPt->get_DefinedFCOs(&mmFcos));
		CreateFcos(mmFcos);
	}
	catch(hresult_exception &) {
	}
}

////////////////////////////// CGuiMetaAttribute /////////////////////////////

#define X1			20
#define X2			150
#define X3			250
#define X4			260
#define ENABLEWIDTH	25
#define Y1			25
#define Y2			45
#define Y3			18
#define GAP			10
#define NUDGE		5

CGuiMetaAttribute::CGuiMetaAttribute(CComPtr<IMgaMetaAttribute> &mgaPt,CGuiMetaFco *o) : owner (o)
{
	CComPtr<IMgaMetaBase> basePt;
	try {
		mgaPt.QueryInterface(&basePt);
		Init(basePt);
		CComBSTR bstr;
		COMTHROW(mgaPt->get_DisplayedName(&bstr));
		CopyTo(bstr,prompt);
	}
	catch(hresult_exception &) {
	}
}

////////////////////////////// CGuiMetaFieldAttribute /////////////////////////////

CGuiMetaFieldAttribute::CGuiMetaFieldAttribute(CComPtr<IMgaMetaAttribute> &mgaPt,CGuiMetaFco *o) : CGuiMetaAttribute(mgaPt,o)
{
	attval_enum tp;
	COMTHROW(mgaPt->get_ValueType(&tp));

	int multiline;
	CComBSTR attrPref;
	CComBSTR attrPrefPath = "multiLine";
	COMTHROW(mgaPt->get_RegistryValue(attrPrefPath, &attrPref));
	CString lines;
	CopyTo(attrPref, lines);
	if (_stscanf(lines, _T("%d"), &multiline) != 1) 
		multiline = 0;
}

////////////////////////////// CGuiMetaToggleAttribute /////////////////////////////

CGuiMetaToggleAttribute::CGuiMetaToggleAttribute(CComPtr<IMgaMetaAttribute> &mgaPt,CGuiMetaFco *o) : CGuiMetaAttribute(mgaPt,o)
{
}

////////////////////////////// CGuiMetaMenuAttribute /////////////////////////////

CGuiMetaMenuAttribute::CGuiMetaMenuAttribute(CComPtr<IMgaMetaAttribute> &mgaPt,CGuiMetaFco *o) : CGuiMetaAttribute(mgaPt,o)
{
	CComPtr<IMgaMetaEnumItems> mgaItems;
	COMTHROW(mgaPt->get_EnumItems(&mgaItems));

	CStringList names;
	CStringList values;

	int i = 0;
	MGACOLL_ITERATE(IMgaMetaEnumItem,mgaItems) {
		CComPtr<IMgaMetaEnumItem> mgaItem;
		mgaItem = MGACOLL_ITER;
		CComBSTR lbl;
		COMTHROW(mgaItem->get_DisplayedName(&lbl));
		CString nm;
		CopyTo(lbl,nm);
		names.AddTail(nm);
		CComBSTR val;
		COMTHROW(mgaItem->get_Value(&val));
		CString vl;
		CopyTo(val,vl);
		values.AddTail(vl);
		i++;
	}
	MGACOLL_ITERATE_END;
}

////////////////////////////// CGuiMetaFco /////////////////////////////

CGuiMetaFco::CGuiMetaFco(CComPtr<IMgaMetaFCO> &mgaPt)
{
	CComPtr<IMgaMetaBase> basePt;
	try {
		mgaPt.QueryInterface(&basePt);
		Init(basePt);
		SetAttributes();
	}
	catch(hresult_exception &) {
	}
}

CGuiMetaFco::CGuiMetaFco(CComPtr<IMgaMetaModel> &mgaPt)
{
	CComPtr<IMgaMetaBase> basePt;
	try {
		mgaPt.QueryInterface(&basePt);
		Init(basePt);
		SetAttributes();
	}
	catch(hresult_exception &) {
	}
}


void CGuiMetaFco::SetAttributes()
{
	try {
		CComPtr<IMgaMetaAttributes> mmAttrs;
		CComPtr<IMgaMetaAttribute> mmAttr;
		CComPtr<IMgaMetaFCO> fco;
		mgaMeta.QueryInterface(&fco);
		COMTHROW(fco->get_Attributes(&mmAttrs));
		MGACOLL_ITERATE(IMgaMetaAttribute,mmAttrs) {
			mmAttr = MGACOLL_ITER;
			attval_enum atype;
			COMTHROW(mmAttr->get_ValueType(&atype));
			CGuiMetaAttribute	*mAttr = 0;
			switch(atype) {
			case ATTVAL_STRING:
			case ATTVAL_INTEGER:
			case ATTVAL_DOUBLE:
				mAttr = new CGuiMetaFieldAttribute(mmAttr,this);
				break;
			case ATTVAL_BOOLEAN:
				mAttr = new CGuiMetaToggleAttribute(mmAttr,this);
				break;
			case _attval_enum:
				mAttr = new CGuiMetaMenuAttribute(mmAttr,this);
				break;
			case ATTVAL_REFERENCE:
			default:
				VERIFY(false);
			}
			attrs.AddTail(mAttr);
		}
		MGACOLL_ITERATE_END;
	}
	catch(hresult_exception &) {
	}
}

////////////////////////////// CGuiMetaModel /////////////////////////////

CGuiMetaModel::CGuiMetaModel(CComPtr<IMgaMetaModel> &mgaPt) : CGuiMetaFco(mgaPt), partsSet(false)
{
	try {
		CComPtr<IMgaMetaFCOs> mmFcos;
		COMTHROW(mgaPt->get_DefinedFCOs(&mmFcos));
		CreateFcos(mmFcos);

		CComPtr<IMgaMetaAspects> mmAspects;
		COMTHROW(mgaPt->get_Aspects(&mmAspects));
		int index = 0;
		MGACOLL_ITERATE(IMgaMetaAspect,mmAspects) {
			CComPtr<IMgaMetaAspect> mmAspect;
			mmAspect = MGACOLL_ITER;
			CGuiMetaAspect *mAspect = new CGuiMetaAspect(mmAspect,this,index++);
			aspects.AddTail(mAspect);
		}
		MGACOLL_ITERATE_END;
		VERIFY(index); // must have at least one aspect defined...

	}
	catch(hresult_exception &) {
	}
}

CGuiMetaModel::~CGuiMetaModel()
{
	DeleteParts();
}

void CGuiMetaModel::DeleteParts()
{
	POSITION pos = parts.GetHeadPosition();
	while(pos)
		delete parts.GetNext(pos);
	parts.RemoveAll();
}

void CGuiMetaModel::InitParts()
{
	CComPtr<IMgaFCO>	nil;
	CComPtr<IMgaMetaModel> mmModel;
	mgaMeta.QueryInterface(&mmModel);
	CComPtr<IMgaMetaRoles> mmRoles;
	COMTHROW(mmModel->get_Roles(&mmRoles));
	MGACOLL_ITERATE(IMgaMetaRole,mmRoles) {
		CComPtr<IMgaMetaRole> mmRole;
		mmRole = MGACOLL_ITER;
		CComPtr<IMgaMetaFCO> fco;
		COMTHROW(mmRole->get_Kind(&fco));
		objtype_enum tp;
		COMTHROW(fco->get_ObjType(&tp));
		CGuiObject *guiObj = 0;
		if(tp == OBJTYPE_MODEL) {
			guiObj = new CGuiModel(nil,mmRole,NULL,NumberOfAspects());
			((CGuiModel *)guiObj)->InitObject(NULL);
			((CGuiModel *)guiObj)->SetAspect(0);
			CSize s;
		}
		else if(tp == OBJTYPE_REFERENCE) {
			guiObj = new CGuiReference(nil,mmRole,NULL,NumberOfAspects(),nil,nil);
			((CGuiReference *)guiObj)->InitObject(NULL);
			((CGuiReference *)guiObj)->SetAspect(0);
		}
		else if(tp != OBJTYPE_CONNECTION) {
			guiObj = new CGuiObject(nil,mmRole,NULL,NumberOfAspects());
			((CGuiObject *)guiObj)->InitObject(NULL);
			((CGuiObject *)guiObj)->SetAspect(0);
		}
		if(!guiObj)
			continue;
		CComBSTR bstr;
		COMTHROW(mmRole->get_DisplayedName(&bstr));
		CopyTo(bstr,guiObj->name);
		guiObj->SetAllSizesToNative();

		parts.AddTail(guiObj);
	}
	MGACOLL_ITERATE_END;
}
	
void CGuiMetaModel::ResetParts()
{
	DeleteParts();
	InitParts();
}

CGuiObjectList &CGuiMetaModel::GetParts()
{
	if(!partsSet) {
		InitParts();
		partsSet = true;
	}
	return parts;
}

void CGuiMetaModel::InitAspectBox(CComboBox *box)
{
	POSITION pos = aspects.GetHeadPosition();
	while(pos) {
		CGuiMetaAspect *asp = aspects.GetNext(pos);
		box->AddString(asp->displayedName);
	}
}

void CGuiMetaModel::InitAspectList(CListBox *list)
{
	POSITION pos = aspects.GetHeadPosition();
	while(pos) {
		CGuiMetaAspect *asp = aspects.GetNext(pos);
		list->AddString(asp->displayedName);
	}
}

CGuiMetaAspect *CGuiMetaModel::GetFirstAspect()
{
	return aspects.GetHead();
}

CGuiMetaAspect *CGuiMetaModel::FindAspect(int ind)
{
	POSITION pos = aspects.GetHeadPosition();
	while(pos) {
		CGuiMetaAspect *asp = aspects.GetNext(pos);
		if(asp->index == ind)
			return asp;
	}
	return 0;
}

CGuiMetaAspect *CGuiMetaModel::FindAspect(CString nm)
{
	POSITION pos = aspects.GetHeadPosition();
	while(pos) {
		CGuiMetaAspect *asp = aspects.GetNext(pos);
		if(asp->name == nm)
			return asp;
	}
	return 0;
}

int CGuiMetaModel::NumberOfAspects()
{
	return aspects.GetCount();
}

/* !!! Verify this code before using it !!! Never used before */
int CGuiMetaModel::MapAspect(CComPtr<IMgaMetaRole> &role,int index)
{
	CComPtr<IMgaMetaModel> parent;
	COMTHROW(role->get_ParentModel(&parent));
	metaref_type metaid;
	COMTHROW(parent->get_MetaRef(&metaid));
	CGuiMetaModel *guiParent = CGuiMetaProject::theInstance->GetGuiMetaModel(metaid);
	VERIFY(guiParent);
	CGuiMetaAspect *aspect = guiParent->FindAspect(index);
	VERIFY(aspect);
	CComPtr<IMgaMetaAspect> metaAspect;
	COMTHROW(aspect->mgaMeta.QueryInterface(&metaAspect));

	CString kindAspect;
	try {
		CComPtr<IMgaMetaParts> mmParts;
		CComPtr<IMgaMetaPart> mmPart;
		COMTHROW(role->get_Parts(&mmParts));

		MGACOLL_ITERATE(IMgaMetaPart,mmParts) {
			mmPart = MGACOLL_ITER;
			CComPtr<IMgaMetaAspect> mmAspect;
			COMTHROW(mmPart->get_ParentAspect(&mmAspect));
			if(IsEqualObject(metaAspect,mmAspect)) {
				CComBSTR bstr;
				COMTHROW(mmPart->get_KindAspect(&bstr));
				CopyTo(bstr,kindAspect);
				if(kindAspect.IsEmpty())
					kindAspect = aspect->name;
				break;
			}
		}
		MGACOLL_ITERATE_END;
	}
	catch(hresult_exception &) {
	}
	VERIFY(!kindAspect.IsEmpty());
	CGuiMetaAspect *found = FindAspect(kindAspect);
	VERIFY(found);
	return found->index;
}

bool CGuiMetaModel::GetPartByRole(CComPtr<IMgaMetaRole> &theRole,CComPtr<IMgaMetaAspect> &theAspect,CComPtr<IMgaMetaPart> &thePart) 
{
	bool res = false;
	try {
		CComPtr<IMgaMetaParts> mmParts;
		CComPtr<IMgaMetaPart> mmPart;
		COMTHROW(theRole->get_Parts(&mmParts));

		MGACOLL_ITERATE(IMgaMetaPart,mmParts) {
			mmPart = MGACOLL_ITER;
			CComPtr<IMgaMetaAspect> mmAspect;
			COMTHROW(mmPart->get_ParentAspect(&mmAspect));
			if( !res && mmAspect == theAspect) {
				thePart = mmPart;
			 	res = true;// we will not return any more from the middle of an iteration
			}
		}
		MGACOLL_ITERATE_END;
	}
	catch(hresult_exception &) {
	}
	return res;
}
////////////////////////////// CGuiMetaAspect /////////////////////////////

CGuiMetaAspect::CGuiMetaAspect(CComPtr<IMgaMetaAspect> &mgaPt, CGuiMetaModel* o, int ind):
	owner(o),
	index(ind)
{
	insertModelMenu = 0;
	insertAtomMenu = 0;
	insertReferenceMenu = 0;
	insertSetMenu = 0;
	CComPtr<IMgaMetaBase> basePt;
	try {
		mgaPt.QueryInterface(&basePt);
		Init(basePt);

		CComPtr<IMgaMetaParts> mmParts;
		CComPtr<IMgaMetaPart> mmPart;
		COMTHROW(mgaPt->get_Parts(&mmParts));
		MGACOLL_ITERATE(IMgaMetaPart,mmParts) {
			mmPart = MGACOLL_ITER;
			VARIANT_BOOL prim;
			COMTHROW(mmPart->get_IsPrimary(&prim));
			if(prim != VARIANT_FALSE) {

				CString roleName = static_cast<const TCHAR*>(mmPart->Role->Name);
				CString label;
				if (mmPart->Role->Kind->Name == mmPart->Role->Name) {
					label = static_cast<const TCHAR*>(mmPart->Role->Kind->DisplayedName);
				}
				else {
					label = static_cast<const TCHAR*>(mmPart->Role->Name);
				}

				objtype_enum type = mmPart->Role->Kind->ObjType;
				switch(type) {
					case OBJTYPE_MODEL:
						if (!insertModelMenu)
							insertModelMenu = new CDynMenu(CGuiMetaProject::theInstance->GetNewMenuCmdID(), _T("Insert New Model"));
						insertModelMenu->AddItem(CGuiMetaProject::theInstance->GetNewMenuCmdID(), std::move(roleName), std::move(label), CString(_T("Help")));
						break;
					case OBJTYPE_ATOM:
						if (!insertAtomMenu)
							insertAtomMenu = new CDynMenu(CGuiMetaProject::theInstance->GetNewMenuCmdID(), _T("Insert New Atom"));
						insertAtomMenu->AddItem(CGuiMetaProject::theInstance->GetNewMenuCmdID(), std::move(roleName), std::move(label), CString(_T("Help")));
						break;
					case OBJTYPE_REFERENCE:
						if (!insertReferenceMenu)
							insertReferenceMenu = new CDynMenu(CGuiMetaProject::theInstance->GetNewMenuCmdID(), _T("Insert New Reference"));
						insertReferenceMenu->AddItem(CGuiMetaProject::theInstance->GetNewMenuCmdID(), std::move(roleName), std::move(label), CString(_T("Help")));
						break;
					case OBJTYPE_SET:
						if (!insertSetMenu)
							insertSetMenu = new CDynMenu(CGuiMetaProject::theInstance->GetNewMenuCmdID(), _T("Insert New Set"));
						insertSetMenu->AddItem(CGuiMetaProject::theInstance->GetNewMenuCmdID(), std::move(roleName), std::move(label), CString(_T("Help")));
						break;
				}
			}
		}
		MGACOLL_ITERATE_END;
	}
	catch(hresult_exception &) {
		// FIXME
	}
	catch (_com_error& ) {
		// FIXME
	}
	if (insertModelMenu)
		insertModelMenu->Sort();
	if (insertAtomMenu)
		insertAtomMenu->Sort();
	if (insertReferenceMenu)
		insertReferenceMenu->Sort();
	if (insertSetMenu)
		insertSetMenu->Sort();

	try {
		CComPtr<IMgaMetaAttributes> mmAttrs;
		CComPtr<IMgaMetaAttribute> mmAttr;
		COMTHROW(mgaPt->get_Attributes(&mmAttrs));
		MGACOLL_ITERATE(IMgaMetaAttribute,mmAttrs) {
			mmAttr = MGACOLL_ITER;
			metaref_type iden;
			COMTHROW(mmAttr->get_MetaRef(&iden));
			CGuiMetaAttribute *guiAttr = CGuiMetaProject::theInstance->GetGuiMetaAttribute(iden);
			VERIFY(guiAttr);
// HACK seems to be screwed: same attribute contained in different objects???
//			VERIFY(guiAttr->owner == owner);
			attrs.AddTail(guiAttr);
		}
		MGACOLL_ITERATE_END;
	}
	catch(hresult_exception &) {
	}
}

CGuiMetaAspect::~CGuiMetaAspect()
{
	delete insertModelMenu;
	delete insertAtomMenu;
	delete insertReferenceMenu;
	delete insertSetMenu;
}

void CGuiMetaAspect::GetMetaAspect(CComPtr<IMgaMetaAspect>& mAspect)
{
	mgaMeta.QueryInterface(&mAspect);
}

void CGuiMetaAspect::InitContextMenu(CMenu* menu, const CString& label, CDynMenu* dm)
{
	if (!dm)
		return;

	unsigned int pos = 0;
	if (FindMenuItem(menu, label, pos)) {
		menu->RemoveMenu(pos, MF_BYPOSITION | MF_POPUP);
		CMenu& dmm = dm->GetMenu();
		menu->InsertMenu(pos, MF_BYPOSITION | MF_POPUP |
			(dm->GetCount() > 0 ? MF_ENABLED : (MF_DISABLED | MF_GRAYED)),
			(UINT_PTR)dmm.m_hMenu, label);
	}

}

void CGuiMetaAspect::InitContextMenu(CMenu* menu)
{
	InitContextMenu(menu,"Insert New Model", insertModelMenu);
	InitContextMenu(menu,"Insert New Atom", insertAtomMenu);
	InitContextMenu(menu,"Insert New Reference", insertReferenceMenu);
	InitContextMenu(menu,"Insert New Set", insertSetMenu);
}

void CGuiMetaAspect::ResetContextMenu(CMenu* menu)
{
	unsigned int pos = 0;
	CString name = "Insert New Model";
	if (FindMenuItem(menu, name, pos))
		menu->RemoveMenu(pos, MF_BYPOSITION | MF_POPUP);
	name = "Insert New Atom";
	if (FindMenuItem(menu, name, pos))
		menu->RemoveMenu(pos, MF_BYPOSITION | MF_POPUP);
	name = "Insert New Reference";
	if (FindMenuItem(menu, name, pos))
		menu->RemoveMenu(pos, MF_BYPOSITION | MF_POPUP);
	name = "Insert New Set";
	if (FindMenuItem(menu, name, pos))
		menu->RemoveMenu(pos, MF_BYPOSITION | MF_POPUP);
}

bool CGuiMetaAspect::FindCommand(int id, CString& label)
{
	CDynMenuItem *dmi = 0;
	if ((insertModelMenu && (dmi = insertModelMenu->FindItem(id)) != 0) ||
		(insertAtomMenu && (dmi = insertAtomMenu->FindItem(id)) != 0) ||
		(insertReferenceMenu && (dmi = insertReferenceMenu->FindItem(id)) != 0) ||
		(insertSetMenu && (dmi = insertSetMenu->FindItem(id)) != 0))
	{
		label = dmi->GetLabel();
		return true;
	}
	return false;
}

bool CGuiMetaAspect::GetRoleByName(const CString& roleName, CComPtr<IMgaMetaRole>& theRole, bool dispName) 
{
	CComPtr<IMgaMetaAspect> aspPt;
	try {
		mgaMeta.QueryInterface(&aspPt);

		CComPtr<IMgaMetaParts> mmParts;
		CComPtr<IMgaMetaPart> mmPart;
		COMTHROW(aspPt->get_Parts(&mmParts));

		MGACOLL_ITERATE(IMgaMetaPart,mmParts) {
			mmPart = MGACOLL_ITER;
			CComPtr<IMgaMetaRole> mmRole;
			COMTHROW(mmPart->get_Role(&mmRole));
			CComBSTR bstr;
			if (dispName) {
				COMTHROW(mmRole->get_DisplayedName(&bstr));
			}
			else {
				COMTHROW(mmRole->get_Name(&bstr));
			}
			CString nm;
			CopyTo(bstr,nm);
			if (nm == roleName) {
				theRole = mmRole;
			 	return true;
			}
		}
		MGACOLL_ITERATE_END;
	}
	catch(hresult_exception &) {
	}
	return false;
}

bool CGuiMetaAspect::GetPartByRole(CComPtr<IMgaMetaRole>& theRole, CComPtr<IMgaMetaPart>& thePart) 
{
	bool res = false;
	CComPtr<IMgaMetaAspect> aspPt;
	try {
		mgaMeta.QueryInterface(&aspPt);

		CComPtr<IMgaMetaParts> mmParts;
		CComPtr<IMgaMetaPart> mmPart;
		COMTHROW(aspPt->get_Parts(&mmParts));

		MGACOLL_ITERATE(IMgaMetaPart,mmParts) {
			mmPart = MGACOLL_ITER;
			CComPtr<IMgaMetaRole> mmRole;
			COMTHROW(mmPart->get_Role(&mmRole));
			if (!res && mmRole == theRole) {
				thePart = mmPart;
			 	res = true; // we will not return any more from the middle of an iteration
			}
		}
		MGACOLL_ITERATE_END;
	}
	catch(hresult_exception &) {
	}
	return res;
}

bool CGuiMetaAspect::IsLinkedRole(CComPtr<IMgaMetaRole>& theRole)
{
	CComPtr<IMgaMetaParts> mmParts;
	COMTHROW(theRole->get_Parts(&mmParts));
	MGACOLL_ITERATE(IMgaMetaPart,mmParts) {
		CComPtr<IMgaMetaPart> mmPart;
		mmPart = MGACOLL_ITER;
		VARIANT_BOOL linked;
		COMTHROW(mmPart->get_IsLinked(&linked));
		if (linked)
			return true;
	}
	MGACOLL_ITERATE_END;
	return false;
}

bool CGuiMetaAspect::IsPrimary(CComPtr<IMgaFCO> fco)
{
	VARIANT_BOOL prim = VARIANT_FALSE;
	CComPtr<IMgaMetaRole> role;
	COMTHROW(fco->get_MetaRole(&role));
	CComPtr<IMgaMetaPart> part;
	if (GetPartByRole(role, part))
		COMTHROW(part->get_IsPrimary(&prim));
	return prim != VARIANT_FALSE;
}

bool CGuiMetaAspect::IsPrimaryByRoleName(CComPtr<IMgaFCO> fco)
{
//	VARIANT_BOOL prim = VARIANT_FALSE;
	CComPtr<IMgaMetaRole> role;
	COMTHROW(fco->get_MetaRole(&role));
	return role == 0 ? false : IsPrimaryByRoleName(role);
}

bool CGuiMetaAspect::IsPrimaryByRoleName(CComPtr<IMgaMetaRole> role)
{
	VARIANT_BOOL prim = VARIANT_FALSE;
	CComBSTR bstr;
	COMTHROW(role->get_Name(&bstr));
	CString roleName;
	CopyTo(bstr,roleName);
	CComPtr<IMgaMetaPart> part;
	CComPtr<IMgaMetaRole> newRole;
	if (GetRoleByName(roleName, newRole) && GetPartByRole(newRole, part))
		COMTHROW(part->get_IsPrimary(&prim));
	return prim != VARIANT_FALSE;
}

bool CGuiMetaAspect::CheckFcosBeforeInsertion(CComPtr<IMgaFCOs> fcos)
{
	bool ok = true;
	MGACOLL_ITERATE(IMgaFCO,fcos) {
		CComPtr<IMgaFCO> fco = MGACOLL_ITER;
		if (!IsPrimary(fco) && !IsPrimaryByRoleName(fco)) {
			ok = false;
			break;
		}
	}
	MGACOLL_ITERATE_END;
	return ok;
}

