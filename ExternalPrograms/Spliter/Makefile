spliter: main.o Spliter.o SpliterUI.o
	gcc -o spliter *.o `fltk-config --ldflags` -lasound -lstdc++

SplitterUI.cxx SpliterUI.h: SpliterUI.fl
	fluid -c SpliterUI.fl

SpliterUI.o: SplitterUI.cxx SpliterUI.h
	gcc -g `fltk-config --cflags` -c SpliterUI.cxx -o SpliterUI.o

Spliter.o: Spliter.C Spliter.h
	gcc -g `fltk-config --cflags` -c Spliter.C -o Spliter.o

main.o: main.C Spliter.h SpliterUI.h
	gcc -g `fltk-config --cflags` -c main.C -o main.o

clean:
	rm -f *.o spliter SpliterUI.cxx SpliterUI.h 
