using System;
using System.Runtime.InteropServices;

class Value {
	enum var_type {
		NIL = 0,
		INTEGER = 1,
		REAL = 2,
		BOOLEAN = 3,
		STRING = 4,
	};
	struct var {
		public var_type type;
		public long d;
		public double f;
		public bool b;
		[MarshalAs(UnmanagedType.LPStr)]
		public string s;
	};

	var[] v = new var[10];

	private const string DLL = "native.dll";
	[DllImport (DLL, CallingConvention=CallingConvention.Cdecl)]
	private static extern
		void invoke(int n, [MarshalAs(UnmanagedType.LPArray, SizeParamIndex=0)] var[] v);

	bool pushvalue(ref var v, object arg) {
		if (arg == null) {
			v.type = var_type.NIL;
		} else {
			Type t = arg.GetType();
			if (t == typeof(int)) {
				v.type = var_type.INTEGER;
				v.d = (int)arg;
			} else if ( t == typeof(long)) {
				v.type = var_type.INTEGER;
				v.d = (long)arg;
			} else if (t == typeof(float)) {
				v.type = var_type.REAL;
				v.f = (float)arg;
			} else if (t == typeof(double)) {
				v.type = var_type.REAL;
				v.f = (double)arg;
			} else if (t == typeof(bool)) {
				v.type = var_type.BOOLEAN;
				v.b = (bool)arg;
			} else if (t == typeof(string)) {
				v.type = var_type.STRING;
				v.s = (string)arg;
			} else {
				return false;
			}
		}
		return true;
	}
	
	public void request(params object[] arg) {
		int n = arg.Length;
		Array.Resize(ref v, n);
		for (int i = 0; i < n; i++) { 
			if (!pushvalue(ref v[i], arg[i])) {
				throw new ArgumentException(String.Format("Unsupport type : {1} at {0}", i, arg[i].GetType()));
			}
		}
		invoke(n, v);
	}
};

public class HelloWorld
{
	static public void Main() {
		Value v = new Value();
		v.request(1,2.0,"hello", false);
	}
}
