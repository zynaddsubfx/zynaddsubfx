clean.sh
fluid -c ControllerUI.fl
gcc -c ControllerUI.cxx -o ControllerUI.o
gcc -c Controller.C -o Controller.o
gcc -c main.C -o main.o



gcc -o controller *.o `fltk-config --ldflags` -lasound -lpthread -lm

