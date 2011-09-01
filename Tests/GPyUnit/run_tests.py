import unittest

_tests = unittest.defaultTestLoader.loadTestsFromNames((
 'test_registrar',
 'GME_297.suite',
 'GME_310.suite',
 'Regr.Mga.tc1',
 'Regr.Mga.tc2',
 'Regr.Mga.tc3',
 'Regr.Mga.tc5',
 'Regr.Mga.tc6',
))

# For debugging
#import sys
#sys.stdin.readline()

if __name__=='__main__':
    import optparse
    parser = optparse.OptionParser()
    parser.add_option("-x", "--xml", action="store_true", dest="xml")
    parser.add_option("-v", action="store_true", dest="verbose")

    (options, args) = parser.parse_args()
    if options.xml:
        import os.path
        with open("tests.xml", "w") as output:
            output.write("<testsuites>")
            for test in _tests:
                runner = xmlrunner.XMLTestRunner(output)
                runner.run(test)
            output.write("</testsuites>")
    else:
        runner = unittest.TextTestRunner()
        runner.run(_tests)
