using System;

public class HelloWorld
{
	// Need code gen
	static string FuncCallByLua(int n, SharpLua.var[] argv) {
		Console.WriteLine("I'm in lua :");
		for (int i=1; i<n; i++) {
			Console.WriteLine("Args {0} type {1}", i, argv[i].type);
		}

		// return string
		// argv[0].type = SharpLua.var_type.STRING;
		// return "ok";

		argv[0].type = SharpLua.var_type.INTEGER;
		argv[0].d = 123;
		return null;
	}

	static public void Main() {
		SharpLua l = new SharpLua("main.lua");
		SharpLua.LuaObject load = l.GetFunction("load");
		SharpLua.LuaObject f = (SharpLua.LuaObject)l.CallFunction(load, "return ...")[0];
		object[] result = l.CallFunction(f, "Hello World", l);
		Console.WriteLine((string)result[0]);
		Console.WriteLine(result[1].GetType());
		SharpLua.LuaObject gc = l.GetFunction("collectgarbage");
		l.CallFunction(gc, "collect");
		l.CollectGarbage();

		SharpLua.LuaObject init = l.GetFunction("init");
		SharpLua.SharpFunction func = FuncCallByLua;
		l.CallFunction(init, func);
		SharpLua.LuaObject callback = l.GetFunction("callback");
		l.CallFunction(callback, 1, null, "string");
	}
}
