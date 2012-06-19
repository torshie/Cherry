#!/usr/bin/env python3

import sys

def getLineLength(line):
	length = 0
	for ch in line:
		if ch == '\t':
			length += 4
		elif ch == '\n':
			return length
		else:
			length += 1
	return length

def checkFile(filename):
	source = open(filename, "r")
	number = 0
	for line in source:
		number += 1
		length = getLineLength(line)
		if length > 75:
			sys.stdout.write("%s:%d\n" % (filename, number))

def main():
	for i in range(1, len(sys.argv)):
		checkFile(sys.argv[i])

if __name__ == "__main__":
	main()
