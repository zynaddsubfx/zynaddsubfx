controller: main.o Controller.o ControllerUI.o
	gcc -o controller *.o `fltk-config --ldflags` -lasound -lpthread -lm -lstdc++

ControllerUI.cxx ControllerUI.h: ControllerUI.fl
	fluid -c ControllerUI.fl

ControllerUI.o: ControllerUI.cxx ControllerUI.h
	gcc `fltk-config --cflags` -c ControllerUI.cxx -o ControllerUI.o

Controller.o: Controller.C Controller.h
	gcc `fltk-config --cflags` -c Controller.C -o Controller.o

main.o: main.C Controller.h ControllerUI.h
	gcc `fltk-config --cflags` -c main.C -o main.o

clean:
	rm -f *.o controller ControllerUI.cxx ControllerUI.h 

