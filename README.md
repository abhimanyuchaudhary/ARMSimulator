================================================
Functional Simulator for subset of ARM Processor
================================================

README

Table of contents
1. Directory Structure
2. How to build
3. How to execute



Directory Structure:
--------------------
CS112-Project
  |
  |- bin
      |
      |- myARMSim
  |- doc
      |
      |- design-doc.docx
      |- Midway Checkpoint.pdf
  |- include
      |
      |- myARMSim.h
  |- src
      |- main.cpp
      |- Makefile
      |- myARMSim.h
  |- test
      |- simple_add.mem
      |- fib.mem
      |- array_add.mem

How to build
------------
For building:
	$cd src
	$make

For cleaning the project:
	$cd src
	$make clean


How to execute
--------------
./myARMSim test/simple_add.mem


References
------------
http://vision.gel.ulaval.ca/~jflalonde/cours/1001/h17/docs/arm-instructionset.pdf

https://www.usebackpack.com/resources/12961/download?1502901940

