all : native.dll test.exe

native.dll : native.c
	gcc -g -Wall -o $@ --shared $^

test.exe : test.cs
	mcs -codepage:utf8 $^

clean:
	rm native.dll test.exe


