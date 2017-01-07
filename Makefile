all : shapelua.dll test.exe

shapelua.dll : shapelua.c
	gcc -g -Wall -o $@ --shared $^ -I/usr/local/include -L/usr/local/lib -llua

test.exe : test.cs shapelua.cs shapeobject.cs
	mcs -codepage:utf8 $^

clean:
	rm shapelua.dll test.exe


