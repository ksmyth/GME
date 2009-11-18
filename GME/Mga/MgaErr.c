// MgaErr.c
// Error table file automatically generated from Interfaces/Mga.idl
// Genarated on Wed Nov 18 15:33:28 2009 
//           by volgy 
//           from E:\GMESRC\GME\Interfaces\Mga.idl
//           to E:\GMESRC\GME\Mga\MgaErr.c
//           using script E:\GMESRC\GME\Mga\scripts\createerrtab.py

const struct errtab MgaErrTab[]= {
{E_MGA_NOT_IMPLEMENTED, L"This method is not yet implemented"},
{E_MGA_NOT_SUPPORTED, L"This mode of operation is not supported"},
{E_MGA_MODULE_INCOMPATIBILITY, L"MGA data structure conflict: error or incompatibility"},
{E_MGA_PARADIGM_NOTREG, L"The paradigm is not registered"},
{E_MGA_PARADIGM_INVALID, L"The paradigm is invalid"},
{E_MGA_COMPONENT_ERROR, L"Some addons cannot be loaded/initialized"},
{E_MGA_DATA_INCONSISTENCY, L"Inconsistent data in database metainfo violated"},
{E_MGA_META_INCOMPATIBILITY, L"Meta incompatibility"},
{E_MGA_PROJECT_OPEN, L"Project must be closed"},
{E_MGA_PROJECT_NOT_OPEN, L"Project must be open"},
{E_MGA_READ_ONLY_ACCESS, L"Database is in read-only mode"},
{E_MGA_NOT_IN_TERRITORY, L"Object is unknown in this territory"},
{E_MGA_NOT_IN_TRANSACTION, L"Operation must execute within transaction"},
{E_MGA_ALREADY_IN_TRANSACTION, L"The project is already in transaction"},
{E_MGA_MUST_ABORT, L"This transaction must be aborted"},
{E_MGA_TARGET_DESTROYED, L"Territory/AddOn already destroyed"},
{E_MGA_FOREIGN_PROJECT, L"This is not the callee object's project"},
{E_MGA_FOREIGN_OBJECT, L"Object does not belong to this project"},
{E_MGA_OBJECT_DELETED, L"The object is deleted partially inaccessible"},
{E_MGA_OBJECT_ZOMBIE, L"The object is deleted and fully inaccessible"},
{E_MGA_ZOMBIE_NOPROJECT, L"The project for this (sub)object has been destroyed"},
{E_MGA_ZOMBIE_CLOSED_PROJECT, L"The project for this (sub)object has been closed"},
{E_MGA_INVALID_ARG, L"invalid argument"},
{E_MGA_ARG_RANGE, L"argument out of range"},
{E_MGA_INPTR_NULL, L"NULL Input COM pointer"},
{E_MGA_OUTPTR_NONEMPTY, L"Nonempty COM pointer supplied for output"},
{E_MGA_OUTPTR_NULL, L"NULL output COM pointer address"},
{E_MGA_NAME_NOT_FOUND, L"Search by name failed"},
{E_MGA_NAME_DUPLICATE, L"The operation caused name uniqueness conflict"},
{E_MGA_META_VIOLATION, L"The operation caused meta violation for object %o"},
{E_MGA_NOT_CHANGEABLE, L"Object cannot be changed"},
{E_MGA_OP_REFUSED, L"Operation refused due to the state of existing data"},
{E_MGA_LIBOBJECT, L"Object belongs to a lib image"},
{E_META_INVALIDATTR, L"Unknown attribute specified"},
{E_META_INVALIDASPECT, L"Unknown aspect specified"},
{E_MGA_ACCESS_TYPE, L"Access method conflicts with attribute type"},
{E_MGA_BAD_ENUMVALUE, L"Invalid value for enum attr"},
{E_MGA_REFPORTS_USED, L"Reference %o is used for connections"},
{E_MGA_VIRTUAL_NODE, L"Node is not a physical, but inherited"},
{E_MGA_BAD_POSITIONVALUE, L"Bad GME position read/written"},
{E_MGA_ROOTFCO, L"Operation not allowed for root FCOs"},
{E_MGA_NOT_ROOTOBJECT, L"Operation not allowed for non-root FCOs"},
{E_MGA_BAD_COLLENGTH, L"Collection parameter counts must match"},
{E_MGA_INVALID_ROLE, L"Bad/NULL metarole or metapart"},
{E_MGA_BAD_ID, L"Object ID format error"},
{E_MGA_NOT_DERIVABLE, L"Non-root object, or derived into itself"},
{E_MGA_OBJECT_NOT_MEMBER, L"%o is not a member in set"},
{E_MGA_NOT_CONNECTIBLE, L"%o is not in a connectible position"},
{E_MGA_NO_ROLE, L"Role not specified"},
{E_MGA_CONNROLE_USED, L"Connrole allowed number exceeded for %o"},
{E_MGA_NOT_DERIVED, L"Object is not derived"},
{E_MGA_NOT_INSTANCE, L"Object is not an instance"},
{E_MGA_INSTANCE, L"Object is a type"},
{E_MGA_REFERENCE_EXPECTED, L"Object %o specified is not a reference"},
{E_MGA_FILTERFORMAT, L"MgaFilter property format error"},
{E_MGA_GEN_OUT_OF_SPACE, L"Cannot generate a unique value"},
{E_MGA_INVALID_TARGET, L"Invalid reference target"},
{E_MGA_LONG_DERIVCHAIN, L"Derive chain too long"},
{E_MGA_LIB_DIFF, L"Invalid changes in library"},
{E_MGA_BAD_MASKVALUE, L"Invalid event mask value"},
{E_MGA_CONSTRAINT_VIOLATION, L"Constraint violation"},
{E_MGA_ERRCODE_ERROR, L"Errcode out of errcode range"},
	{0}};
