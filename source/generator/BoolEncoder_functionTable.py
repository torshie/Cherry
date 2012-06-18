#!/usr/bin/env python3

import sys
import cherry

def main():
	sys.stdout.write(cherry.donnotModifyWarning())
	sys.stdout.write("""
#include <cherry/encode/BoolEncoder.hpp>

using namespace cherry;

BoolEncoder::EncodingFunction BoolEncoder::functionTable[256] = {
""")
	for i in range(0, 255):
		sys.stdout.write("\t&(BoolEncoder::addByte<%d>), \n" % (i,))
	sys.stdout.write("\t&(BoolEncoder::addByte<255>)\n")
	sys.stdout.write("}; // functionTable\n")

if __name__ == "__main__":
	main()
