import unittest
import os
import GPyUnit.util

def _adjacent_file(file):
    import os.path
    return os.path.join(os.path.dirname(os.path.abspath(__file__)), file)

class TestParser(unittest.TestCase):
    def test_ParseMetaGME(self):
        mga = GPyUnit.util.parse_xme(self.connstr)
        mga.Save()
        mga.Close()
    
    @property
    def connstr(self):
        return "MGA=" + _adjacent_file("tmp.mga")

GPyUnit.util.MUGenerator(globals(), TestParser)

if __name__ == "__main__":
        unittest.main()
