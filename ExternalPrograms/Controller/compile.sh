./clean.sh
fluid -c ControllerUI.fl
gcc `fltk-config --cflags` -c ControllerUI.cxx -o ControllerUI.o
gcc `fltk-config --cflags` -c Controller.C -o Controller.o
gcc `fltk-config --cflags` -c main.C -o main.o



gcc -o controller *.o `fltk-config --ldflags` -lasound -lpthread -lm

