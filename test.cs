using System;

public class HelloWorld
{
	static public void Main() {
		ShapeLua l = new ShapeLua("main.lua");
		ShapeLua.LuaObject load = l.GetFunction("load");
		ShapeLua.LuaObject f = (ShapeLua.LuaObject)l.CallFunction(load, "return ...")[0];
		object[] result = l.CallFunction(f, "Hello World", l);
		Console.WriteLine((string)result[0]);
		Console.WriteLine(result[1].GetType());
		ShapeLua.LuaObject gc = l.GetFunction("collectgarbage");
		l.CallFunction(gc, "collect");
		l.CollectGarbage();
	}
}
