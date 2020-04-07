import unittest
import os
import win32com.client
import GPyUnit.util
from GPyUnit.util import dec_disable_early_binding
from GPyUnit.util import DispatchEx

def _adjacent_file(file):
    import os.path
    return os.path.join(os.path.dirname(os.path.abspath(__file__)), file)

class CoreAbortTransaction(unittest.TestCase):
    def testTerritoryDestroy(self):
        project = DispatchEx('Mga.MgaProject')
        project.Create(self.connstr, 'MetaGME')
        paradigmSheet = project.RootMeta.RootFolder.DefinedFCOByName('ParadigmSheet', True)
        project.BeginTransactionInNewTerr()
        base = project.RootFolder.CreateRootObject(paradigmSheet)
        base.CreateChildObject(paradigmSheet.RoleByName('Atom'))
        for i in range(8):
            base.ParentFolder.DeriveRootObject(base, True)
        base_id = base.ID
        project.CommitTransaction()
        project.Close(False)
        
        project = DispatchEx('Mga.MgaProject')
        project.Open(self.connstr)

        assert project.UndoRedoSize() == (0,0)

        oldpref = project.Preferences
        project.Preferences = 0x00000080
        terr = project.BeginTransactionInNewTerr()
        base = project.GetObjectByID(base_id)
        derived = base.DerivedObjects.Item(2)
        base.SetRegistryValueDisp('random', 'asdf')
        base.Name = 'super'
        project.CommitTransaction()
        project.Preferences = oldpref

        terr2 = project.BeginTransactionInNewTerr()
        terr.Destroy() # puts locking in a bad state pre-r2234 (before CoreTerritory gutting)
        derived = terr2.OpenFCO(derived)
        derived.DestroyObject()
        project.AbortTransaction()

        terr3 = project.BeginTransactionInNewTerr()
        base = terr3.OpenFCO(base)
        base.DestroyObject()
        project.CommitTransaction() # crashes here pre-r2234 (before CoreTerritory gutting)
        project.Close(True)

    def testNestedTxRollback(self):
        from pythoncom import com_error
        project = DispatchEx('Mga.MgaProject')
        project.Create(self.connstr, 'MetaGME')
        paradigmSheet = project.RootMeta.RootFolder.DefinedFCOByName('ParadigmSheet', True)
        project.BeginTransactionInNewTerr()
        model = project.RootFolder.CreateRootObject(paradigmSheet)
        atom = model.CreateChildObject(paradigmSheet.RoleByName('Atom'))
        conn = model.CreateChildObject(paradigmSheet.RoleByName('Connection'))
        conn.Name = 'Connection'
        connProxy = model.CreateChildObject(paradigmSheet.RoleByName('ConnectionProxy'))
        connProxy.Name = 'ConnectionProxy'
        connProxy.Referred = conn
        self.assertRaises(com_error, setattr, connProxy, 'Referred', atom) # this triggers a nested tx rollback
        atom.Name = 'Atom'
        project.CommitTransaction()

        def check():
            terr = project.BeginTransactionInNewTerr()
            atom_ = terr.OpenFCO(atom)
            self.assertEqual('Atom', atom_.Name)
            conn_ = terr.OpenFCO(conn)
            self.assertEqual('Connection', conn_.Name)
            connProxy_ = terr.OpenFCO(connProxy)
            self.assertEqual('ConnectionProxy', connProxy_.Name)
            self.assertEqual(conn_, connProxy_.Referred)
            project.CommitTransaction()
        check()

        terr = project.BeginTransactionInNewTerr()
        model = terr.OpenFCO(model)
        atom = terr.OpenFCO(atom)
        atom.Name = ''
        self.assertRaises(com_error, getattr(model, 'CreateSimpleConnDisp'), paradigmSheet.RoleByName('ReferTo'), atom, atom, None, None)
        atom.Name = 'Atom'
        project.CommitTransaction()

        check()

        project.Close(False)
        del(terr)
    
    def testCloseWithOpenTx(self):
        project = DispatchEx('Mga.MgaProject')
        project.Create(self.connstr, 'MetaGME')
        paradigmSheet = project.RootMeta.RootFolder.DefinedFCOByName('ParadigmSheet', True)
        terr = project.BeginTransactionInNewTerr()
        base = project.RootFolder.CreateRootObject(paradigmSheet)
        base.CreateChildObject(paradigmSheet.RoleByName('Atom'))
        for i in range(8):
            base.ParentFolder.DeriveRootObject(base, True)
        base_id = base.ID
        # don't: project.CommitTransaction()
        project.Close(True)
        del(terr)

    def testModificationDetection(self):
        project = DispatchEx('Mga.MgaProject')
        project.Create(self.connstr, 'MetaGME')
        paradigmSheet = project.RootMeta.RootFolder.DefinedFCOByName('ParadigmSheet', True)
        self.assertEquals(project.ProjectStatus & 4, 4)
        project.Preferences = 0x00000080
        project.BeginTransactionInNewTerr()
        base = project.RootFolder.CreateRootObject(paradigmSheet)
        base.CreateChildObject(paradigmSheet.RoleByName('Atom'))
        for i in range(20):
            base.ParentFolder.DeriveRootObject(base, True)
        base_id = base.ID
        project.CommitTransaction()

        self.assertEquals(project.UndoRedoSize(), (1, 0))
        self.assertEquals(project.ProjectStatus & 4, 4)

        for i in range(1, 20):
            terr = project.BeginTransactionInNewTerr()
            base = project.GetObjectByID(base_id)
            derived = base.DerivedObjects.Item(i)
            base.SetRegistryValueDisp('random', 'asdf')
            base.Name = 'super'
            project.CommitTransaction()
            self.assertEquals(project.ProjectStatus & 4, 4)
            if i == 11:
                project.Save(project.ProjectConnStr, False)
                self.assertEquals(project.ProjectStatus & 4, 0)
        for i in range(7):
            project.Undo()
            self.assertEquals(project.ProjectStatus & 4, 4)
        project.Undo()
        self.assertEquals(project.ProjectStatus & 4, 0)
        project.Redo()
        self.assertEquals(project.ProjectStatus & 4, 4)
        project.Undo()
        self.assertEquals(project.ProjectStatus & 4, 0)
        project.Undo()
        self.assertEquals(project.ProjectStatus & 4, 4)
        project.Redo()
        self.assertEquals(project.ProjectStatus & 4, 0)
        project.Close(True)


    @property
    def connstr(self):
        return 'MGA=' + _adjacent_file('tmp.mga')
