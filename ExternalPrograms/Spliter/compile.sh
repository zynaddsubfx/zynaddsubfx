./clean.sh
fluid -c SpliterUI.fl
gcc `fltk-config --cflags` -c SpliterUI.cxx -o SpliterUI.o
gcc `fltk-config --cflags` -c Spliter.C -o Spliter.o
gcc `fltk-config --cflags` -c main.C -o main.o

gcc -o spliter *.o `fltk-config --ldflags` -lasound

