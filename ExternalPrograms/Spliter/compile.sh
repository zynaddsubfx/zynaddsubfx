clean.sh
fluid -c SpliterUI.fl
gcc -c SpliterUI.cxx -o SpliterUI.o
gcc -c Spliter.C -o Spliter.o
gcc -c main.C -o main.o

gcc -o spliter *.o `fltk-config --ldflags` -lasound

