cxxtestgen.py --error-printer -o runner.cpp SampleTest.h
g++ -g -o runner runner.cpp ../Samples/AuSample.o ../Samples/Sample.o
