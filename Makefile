all : sharplua.dll test.exe

sharplua.dll : sharplua.c
	gcc -g -Wall -o $@ --shared $^ -I/usr/local/include -L/usr/local/lib -llua

test.exe : test.cs sharplua.cs sharpobject.cs
	mcs -codepage:utf8 $^

clean:
	rm sharplua.dll test.exe


