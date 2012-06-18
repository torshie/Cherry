import os.path
import sys

def donnotModifyWarning():
	return """
/********************** DO NOT MODIFY THIS FILE ************************
              This file is generated by %s
 ***********************************************************************/
""" % (os.path.basename(sys.argv[0]),)