cd ../
make
cd Tests
cxxtestgen.py --error-printer -o runner.cpp SampleTest.h EchoTest.h \
ControllerTest.h MicrotonalTest.h XMLwrapperTest.h
g++ -g -o runner runner.cpp ../Samples/AuSample.o ../Samples/Sample.o ../Effects/Echo.o ../Effects/Effect.o ../Controls/Control.o ../Controls/DelayCtl.o ../Params/Controller.o ../Misc/XMLwrapper.o ../Misc/Config.o ../Misc/Util.o ../Misc/Microtonal.o -lmxml -lm -lz -lpthread
