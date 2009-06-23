// SearchAlg.cpp: implementation of the CSearch class.
//
//////////////////////////////////////////////////////////////////////


#include "stdafx.h"
#include "ComHelp.h"
#include "Input.h"
#include "SearchAlg.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CSearch::CSearch(CInput inp)
{
    filter = inp;
}


CSearch::~CSearch()
{
}

// the main entry point for a search
void CSearch::Search(IMgaFolder *root, IMgaObjects* scopeColl, IMgaFCO *selected, CComPtr<IMgaFCOs> disp, CProgressCtrl *Progress)
{
    results = disp;
    m_pgsSearch = Progress;
    CComPtr<IMgaFolder> pRoot = root;


    if( filter.GetSearchScope()==1 && scopeColl)
    {
        CComPtr<IMgaFCOs> non_container_coll;                  // will store the Atoms, Refs and Sets
        non_container_coll.CoCreateInstance( L"Mga.MgaFCOs");  // which are not Fs and Ms
        long nExtra = 0;                                       // how many are there in the above collection

        MGACOLL_ITERATE(IMgaObject, scopeColl)
        {
            CComPtr<IMgaObject> iter;

            objtype_enum type;
            COMTHROW( MGACOLL_ITER->get_ObjType( &type));

            switch( type) {
                case OBJTYPE_FOLDER:   SearchFolderHierarchy( CComQIPtr<IMgaFolder>( MGACOLL_ITER));break;
                case OBJTYPE_MODEL:    SearchModelHierarchy( CComQIPtr<IMgaModel>( MGACOLL_ITER));break;
                case OBJTYPE_SET:
                case OBJTYPE_ATOM:
                case OBJTYPE_REFERENCE: 
                    CComQIPtr<IMgaFCO> an_fco( MGACOLL_ITER);
                    if( an_fco && non_container_coll) 
                    {
                        non_container_coll->Append( an_fco);
                        ++nExtra;
                    }
            } // switch
        } MGACOLL_ITERATE_END;

        if( nExtra && non_container_coll) // if coll is non-zero and has elements inside
        {		
            CheckAllReferences( non_container_coll);  // deals with refs only
            CheckAllAtoms( non_container_coll);       // ... atoms only
            CheckAllSets( non_container_coll);        // ... sets only
        }
        non_container_coll.Release();
    }
    else if(filter.GetSearchScope()==0)
    {
        SearchFolderHierarchy(pRoot);	
    }

    //return results;
}

// get all the atoms that have a matching role name, calling CheckAtom on each
void CSearch::CheckAllAtoms(IMgaFCOs *allObjects)
{

    MGACOLL_ITERATE(IMgaFCO, allObjects)
    {
        objtype_enum rret;
        COMTHROW( MGACOLL_ITER->get_ObjType( &rret) );

        if(rret == OBJTYPE_ATOM)  
        {
            if (CheckAtom((IMgaAtom*)(MGACOLL_ITER.p))) COMTHROW(results->Append((IMgaAtom*)(MGACOLL_ITER.p)));
        }

        m_pgsSearch->StepIt();	
    } MGACOLL_ITERATE_END;

}

// get all the references that have a matching role name, calling CheckReference on each
void CSearch::CheckAllReferences(IMgaFCOs *allObjects)
{

    MGACOLL_ITERATE(IMgaFCO, allObjects)
    {
        objtype_enum rret;
        COMTHROW( MGACOLL_ITER->get_ObjType( &rret) );

        if(rret == OBJTYPE_REFERENCE )
        {
            if (CheckAtom((IMgaAtom*)(MGACOLL_ITER.p))) COMTHROW(results->Append((IMgaReference*)(MGACOLL_ITER.p)));
        }

        m_pgsSearch->StepIt();
    } MGACOLL_ITERATE_END;

}

// get all the sets that have a matching role name, calling CheckSet on each
void CSearch::CheckAllSets(IMgaFCOs *allObjects)
{

    MGACOLL_ITERATE(IMgaFCO, allObjects)
    {
        objtype_enum rret;
        COMTHROW( MGACOLL_ITER->get_ObjType( &rret) );

        if(rret == OBJTYPE_SET)
        {
            if(CheckSet((IMgaSet*)(MGACOLL_ITER.p)))COMTHROW(results->Append((IMgaSet*)(MGACOLL_ITER.p)));
        }

        m_pgsSearch->StepIt();
    } MGACOLL_ITERATE_END;	
}

// get all the models that have a matching role name, calling CheckModel on each
void CSearch::CheckAllModels(IMgaFCOs *allObjects)
{
    MGACOLL_ITERATE(IMgaFCO, allObjects)
    {
        objtype_enum rret;
        COMTHROW( MGACOLL_ITER->get_ObjType( &rret) );

        if(rret == OBJTYPE_MODEL) 
        {
            if(CheckModel((IMgaModel *)(MGACOLL_ITER.p))) COMTHROW(results->Append((IMgaModel*)(MGACOLL_ITER.p)));;
        }

        m_pgsSearch->StepIt();

    } MGACOLL_ITERATE_END;
}

//searches the folder hierarchy, first searches all the child models and modelhierarchies of the child models
//then does the same with each child folder, then searches the folderhiearchy of each child folder's child folders
//for a Breadth First Search (not quite perfectly BFS)
void CSearch::SearchFolderHierarchy(IMgaFolder *root)
{
    // Search in Root Models & Below
    CComPtrList<IMgaModel> *rootmodlist = new CComPtrList<IMgaModel>; //needed to use new or the addtail below would fail	
    CComPtr<IMgaFCOs> children;
    COMTHROW( root->get_ChildFCOs(&children));
    MGACOLL_ITERATE(IMgaFCO, children) {
        CComPtr<IMgaModel> mm;
        MGACOLL_ITER.QueryInterface(&mm);
        if(mm != NULL) 
        {
            rootmodlist->AddTail( mm );
        }
    } MGACOLL_ITERATE_END;

    if (filter.GetModels()) 
    {
        CheckAllModels(children);
    }

    if (filter.GetAtoms())
    {
        CheckAllAtoms(children);
    }

    if (filter.GetReferences())
    {
        CheckAllReferences(children);
    }

    if (filter.GetSets())
    {
        CheckAllSets(children);
    } 

    POSITION rmpos = rootmodlist->GetHeadPosition();
    while(rmpos)
    {
        CComPtr<IMgaModel> rootmodel = rootmodlist->GetNext(rmpos);
        SearchModelHierarchy(rootmodel);
    }

    delete rootmodlist; //no longer needed, (used new above)

    // Search in Folders & Below
    CComPtr<IMgaFolders> flist;
    COMTHROW( root->get_ChildFolders(&flist));

    MGACOLL_ITERATE(IMgaFolder, flist)
    {
        // Search in Folder-Models & Below


        //uncomment next line, and comment out rest of function for more dfs approach
        //		SearchFolderHierarchy(MGACOLL_ITER);

        CComPtrList<IMgaModel> *mlist = new CComPtrList<IMgaModel>;	//needed to use new or the addtail below would fail
        CComPtr<IMgaFCOs> subchildren;
        COMTHROW( MGACOLL_ITER->get_ChildFCOs(&subchildren));
        MGACOLL_ITERATE(IMgaFCO, subchildren) {
            CComPtr<IMgaModel> mod;
            MGACOLL_ITER.QueryInterface(&mod);
            if(mod != NULL) 
            {
                mlist->AddTail( mod );
            }
        } MGACOLL_ITERATE_END;

        if (filter.GetModels()) 
        {
            CheckAllModels(subchildren);
        }

        if (filter.GetAtoms())
        {
            CheckAllAtoms(subchildren);
        }

        if (filter.GetReferences())
        {
            CheckAllReferences(subchildren);
        }

        if (filter.GetSets())
        {
            CheckAllSets(subchildren);
        } 

        POSITION mpos = mlist->GetHeadPosition();
        while(mpos)
        {
            CComPtr<IMgaModel> submodel = mlist->GetNext(mpos);
            SearchModelHierarchy(submodel);
        }

        delete mlist; //no longer needed, (used new above)

        // Search in SubFolders & Below
        CComPtr<IMgaFolders> sflist;
        COMTHROW( MGACOLL_ITER->get_ChildFolders(&sflist));
        MGACOLL_ITERATE(IMgaFolder, sflist)
        {
            SearchFolderHierarchy(MGACOLL_ITER);
        } MGACOLL_ITERATE_END; 

    } MGACOLL_ITERATE_END;
}


//checks all the children of the input model, then searches the model hierarchy of all the child models
void CSearch::SearchModelHierarchy(IMgaModel *root)
{
    if(root != NULL)
    {
        CComPtr<IMgaFCOs> modelChildren;
        COMTHROW(root->get_ChildFCOs(&modelChildren));

        if (filter.GetAtoms())
        {
            CheckAllAtoms(modelChildren);
        }

        if (filter.GetReferences())
        {
            CheckAllReferences(modelChildren);
        }

        if (filter.GetSets())
        {
            CheckAllSets(modelChildren);
        } 

        if (filter.GetModels())
        {
            CheckAllModels(modelChildren);
        }

        CComPtrList<IMgaModel> submodellist;
        CComPtr<IMgaFCOs> psa;
        COMTHROW( root->get_ChildFCOs(&psa));
        MGACOLL_ITERATE(IMgaFCO, psa) {
            CComPtr<IMgaModel> mm;
            MGACOLL_ITER.QueryInterface(&mm);
            if(mm != NULL) 
            {
                submodellist.AddTail( mm );
            }
        } MGACOLL_ITERATE_END;

        POSITION smpos = submodellist.GetHeadPosition();

        while(smpos)
        {
            CComPtr<IMgaModel> submodel = submodellist.GetNext(smpos);

            SearchModelHierarchy(submodel);
        }
    }
}

//checks the name, kindname and attributes, adding all matches to the results
bool CSearch::CheckAtom(IMgaFCO *Atom)
{

    //check if the first search criteria matches
    int x=Matches(Atom,true);

    //check if the atom matches second search criteria
    int y=Matches(Atom,false);

    //perform logical operation to determine if bot the result match overall criteria   
    return PerformLogical(x,y);

}

//checks the name, kindname and attributes, adding all matches to the results
bool CSearch::CheckReference(IMgaFCO *Reference)
{

    //check if the References satisfies first search criteria
    int x=Matches(Reference,true);

    //check if the reference matches second search criteria
    int y=Matches(Reference,false);

    return PerformLogical(x,y);
}

//checks the name, kindname and attributes, adding all matches to the results
bool CSearch::CheckSet(IMgaFCO *Set)
{

    //check if the first criteria is satisfied
    int x=Matches(Set,true);

    //check if second criteria is satisified
    int y=Matches(Set,false);

    //check if logical combination is satisfied
    return PerformLogical(x,y);
}

//checks the name, kindname and attributes, adding all matches to the results
bool CSearch::CheckModel(IMgaFCO *Model) //BOOL first added to check if it matches first or second
{
     //check if the first criteria is satisfied
    int x=Matches(Model,true);

     //check if second criteria is satisified
    int y=Matches(Model,false);

    //check if logical combination is satisfied
    return PerformLogical(x,y);
}

//special search for references to specific objects
void CSearch::SearchReferences(IMgaFCO *referenced)
{
    CComPtr<IMgaFCOs> objRefByList;
    COMTHROW(referenced->get_ReferencedBy(&objRefByList));

    MGACOLL_ITERATE(IMgaFCO, objRefByList)
    {
        CComPtr<IMgaFCO> refobj = MGACOLL_ITER;
        COMTHROW(results->Append(refobj));
    } MGACOLL_ITERATE_END;
}

bool CSearch::CheckAttributes(IMgaFCO *obj,bool first)
{	
    int index=0;
    	
    bool found = false;
    CComPtr<IMgaFCO> cObj = obj;

    CStringList attributeList;

    //stores type of the attribute
    std::vector<attval_enum> typeList;

    //stores indices of the attribute in expression stack to access it randomly
    //all attributes in the expression stack may not match the criteria
    //so for the ones matching the criteria the index is stored so that it can be accessed 
    //easily
    std::vector<int> indices;

    //Get appropriate expression stack, either first or second 
    std::vector<Attribute> expressionStack=first?filter.GetFirstAttributeStack():filter.GetSecondAttributeStack();

    CComPtr<IMgaMetaFCO> cmeta;
    CComPtr<IMgaMetaAttributes> mattrs;
    COMTHROW(cObj->get_Meta(&cmeta));
    COMTHROW(cmeta->get_Attributes(&mattrs));

    index = 0;

    //iterate thru attributes and make a list if it matches search criteria
    //also store its type and the index in original expression stack
    MGACOLL_ITERATE(IMgaMetaAttribute, mattrs) {
        attval_enum type;
		CComBSTR strDisplayedName;
        COMTHROW(MGACOLL_ITER->get_DisplayedName(&strDisplayedName) );
		CComBSTR strName;
		COMTHROW(MGACOLL_ITER->get_Name(&strName) );
        index=0; 
        for(std::vector<Attribute>::iterator it=expressionStack.begin();it!=expressionStack.end();++it)
        {
            Attribute attr=*it;
            CString name=(CString) strDisplayedName;
            if (filter.IsCaseIgnored()) name.MakeLower();
            if(std::tr1::regex_search((LPCTSTR)name,attr.GetRegExp(attr.name,filter.MatchWholeWord())))
            {
                COMTHROW(MGACOLL_ITER->get_ValueType(&type) );
                attributeList.AddTail(CString(strName));

                //put the attribute type in a vector
                typeList.push_back(type);

                //also put the appropriate index of the attribute name in the list
                indices.push_back(index);
            }
            index++;
        }
    } MGACOLL_ITERATE_END;


    //now check the attributes one by one
    POSITION strpos = attributeList.GetHeadPosition();
    index=0;
  
    while(strpos)
    {
        CString strAttribute = attributeList.GetNext(strpos);
        
        //aceess the corresponding attribute by using the index stored in a vector
        Attribute& attribute = expressionStack[indices[index]];


        CString objVal=expressionStack[indices[index]].value;
        if(!objVal.IsEmpty())
        {
            //place holders for actual attribute values
            int intActualValue;
            double dblActualValue;
            bool bActualValue;
            CString strActualValue;

            //User supplied values
            int intSearchValue;
            double dblSearchValue;
            bool bSearchValue;	

            CBstr bstrS;
            long value;
            VARIANT_BOOL vb;

            switch(typeList[index])
            {
            case ATTVAL_STRING:		
                if(! SUCCEEDED( cObj->get_StrAttrByName((CBstrIn)strAttribute, bstrS) ) ) attribute.eval=FALSE;

                strActualValue = bstrS;
                if( filter.IsCaseIgnored()) // case ignored, make attrib value lowercase
                {
                    strActualValue.MakeLower();
                }

                if(attribute.CheckString(std::tr1::regex_search((LPCTSTR)strActualValue,attribute.GetRegExp(objVal,filter.MatchWholeWord()))))
                    attribute.eval=TRUE;
                break;

            case ATTVAL_INTEGER:	

                if(! SUCCEEDED( cObj->get_IntAttrByName((CBstrIn)strAttribute, &value) ) ) attribute.eval=FALSE;

                intActualValue = value;

                intSearchValue = atoi(attribute.value);
                if(attribute.CheckInteger(intActualValue,intSearchValue))
                    attribute.eval=TRUE;
                break;

            case ATTVAL_DOUBLE:		

                if( !SUCCEEDED( cObj->get_FloatAttrByName((CBstrIn)strAttribute, &dblActualValue) ) ) attribute.eval=FALSE;

                dblSearchValue = atof(attribute.value);
                if(attribute.CheckDouble(dblActualValue,dblSearchValue))
                    attribute.eval=TRUE ;
                break;

            case ATTVAL_BOOLEAN:	

                if( !SUCCEEDED( cObj->get_BoolAttrByName((CBstrIn)strAttribute, &vb) ) ) attribute.eval=FALSE;

                bActualValue = (vb != 0);			
                intSearchValue = atoi(attribute.value);
                if(attribute.value=="false" || attribute.value=="False" || attribute.value=="FALSE" || intSearchValue==0)
                    bSearchValue = false;
                if(attribute.value=="true" || attribute.value=="True" || attribute.value=="TRUE" || intSearchValue==1)
                    bSearchValue = true;

                if(attribute.CheckBool(bActualValue,bSearchValue))
                    attribute.eval=TRUE;
                break;

            case ATTVAL_REFERENCE:	break;

            default:				
                attribute.eval = TRUE; break;
            }
        }
        else
            attribute.eval = TRUE;

        
        ++index;
    }		
    return EvaluateResult(expressionStack); 
}

//Evaluate the logical combination result

bool CSearch::EvaluateResult(std::vector<Attribute>& vectorAttributes)
{
    if(vectorAttributes.size()==0)
        return true;
    else if (vectorAttributes.size()==1)
        return (vectorAttributes[0].eval==TRUE);

    while(vectorAttributes.size()!=1)
    {
        //pop the first attribute
        Attribute attribute1=vectorAttributes.back();
        vectorAttributes.pop_back();

        //pop the operation
        Attribute operation=vectorAttributes.back();
        vectorAttributes.pop_back();

        //pop the second attribute
        Attribute attribute2=vectorAttributes.back();
        vectorAttributes.pop_back();

        //binary evaluation of 2 attributes using the operation
        attribute1.eval=operation.LogicalCompare(attribute1.eval,operation,attribute2.eval);

        //put the result back
        vectorAttributes.push_back(attribute1);
    }
    return (vectorAttributes[0].eval==TRUE);
}


void CSearch::SearchResults(CComPtr<IMgaFCOs> old_results,CComPtr<IMgaFCOs> disp, CProgressCtrl *Progress)
{
    //CComPtr<IMgaFCO> fco;
    results=disp;
    m_pgsSearch=Progress;


    MGACOLL_ITERATE(IMgaFCO, old_results)
    {
        objtype_enum rret;
        COMTHROW( MGACOLL_ITER->get_ObjType( &rret) );
        if (rret == OBJTYPE_ATOM  && filter.GetAtoms())
        {
            if (CheckAtom((IMgaAtom*)(MGACOLL_ITER.p))) COMTHROW(results->Append((IMgaAtom*)(MGACOLL_ITER.p)));
        }
        else if (rret == OBJTYPE_MODEL && filter.GetModels())
        {
            if(CheckModel((IMgaModel *)(MGACOLL_ITER.p))) COMTHROW(results->Append((IMgaModel*)(MGACOLL_ITER.p)));;
        }
        else if (rret == OBJTYPE_REFERENCE && filter.GetReferences())
        {
            if (CheckAtom((IMgaAtom*)(MGACOLL_ITER.p))) COMTHROW(results->Append((IMgaReference*)(MGACOLL_ITER.p)));
        }
        else if (rret == OBJTYPE_SET && filter.GetSets())
        {
            if(CheckSet((IMgaSet*)(MGACOLL_ITER.p)))COMTHROW(results->Append((IMgaSet*)(MGACOLL_ITER.p)));
        }
    } MGACOLL_ITERATE_END;
}


//Perform logical operations ANDing and ORing
//to evaluate whether the final result is true

bool CSearch::PerformLogical(int x,int y)
{
    //-1 undefined, 0 -false, 1 - true
    //-1 is used to indicate that no search term is supplied in the 
    //corresponding field
    switch (filter.GetLogicalOperator())
    {
     //and
    case 0:
        return (x==-1 && y==1)||(x==1 && y==-1) ||(x==1&&y==1);
     //or
    case 1:
        return x==1 || y==1;
    //xor
    case 2:
        return (x==-1 && y==1)||(x==1 && y==-1)||(x==1 && y==0)||(x==0 && y==1);
    }

    return false;
}

int CSearch::Matches(IMgaFCO* fco,bool first)
{
    CString partName;
    CBstr bstr;
    CComPtr<IMgaMetaRole> rmeta;

    //get role
    COMTHROW( fco->get_MetaRole(&rmeta) );
    if(rmeta) 
    {
        COMTHROW( rmeta->get_Name(bstr) );
        partName = bstr;
    } 
    else 
        partName = "";

    //get the name
    CBstr bstrName;
    COMTHROW(fco->get_Name(bstrName));
    CString strName = bstrName;

    //get the KindName
    CBstr bstrKindName;
    CComPtr<IMgaMetaFCO> cmeta;
    COMTHROW( fco->get_Meta(&cmeta) );
    COMTHROW( fco->get_Name(bstrKindName) );
    CString kindName = bstrKindName;

    if( filter.IsCaseIgnored()) // case ignored, make values lowercase
    {
        strName.MakeLower();
        kindName.MakeLower();
        partName.MakeLower();
    }
    bool found = true;
    int result=-1;
    if(first)
    {
        if(!filter.GetFirstRole().IsEmpty())
            result=std::tr1::regex_search((LPCTSTR)partName,filter.GetFirstRoleRegExp())?1:0;

        if(!filter.GetFirstKind().IsEmpty() && result!=0)
            result=std::tr1::regex_search((LPCTSTR)kindName,filter.GetFirstKindRegExp())?1:0;

        if(!filter.GetFirstName().IsEmpty() &&result!=0)
            result=std::tr1::regex_search((LPCTSTR)strName,filter.GetFirstNameRegExp())?1:0;

        if(!filter.GetFirstAttribute().IsEmpty() &&result!=0)
            result=CheckAttributes(fco,first)?1:0;
    }
    else
    {
        if(!filter.GetSecondRole().IsEmpty())
            result=std::tr1::regex_search((LPCTSTR)partName,filter.GetSecondRoleRegExp())?1:0;

        if(!filter.GetSecondKind().IsEmpty()&& result!=0)
            result=std::tr1::regex_search((LPCTSTR)kindName,filter.GetSecondKindRegExp())?1:0;

        if(!filter.GetSecondName().IsEmpty() &&result!=0)
            result=std::tr1::regex_search((LPCTSTR)strName,filter.GetSecondNameRegExp())?1:0;

        if(!filter.GetSecondAttribute().IsEmpty() && result!=0)
            result=CheckAttributes(fco,first)?1:0;
    }

    return result;
}
