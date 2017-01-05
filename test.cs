using System;
using System.Runtime.InteropServices;

class Value {
	enum var_type {
		NIL = 0,
		INTEGER = 1,
		INT64 = 2,
		REAL = 3,
		BOOLEAN = 4,
		STRING = 5,
	};
	struct var {
		public int type;
		public int d;
		public long d64;
		public double f;
	};
	
	const int default_length = 10;
	var[] v = new var[default_length];
	string[] vs = new string[default_length];

	const string DLL = "native.dll";
	[DllImport (DLL, CallingConvention=CallingConvention.Cdecl)]
	static extern
		void invoke(int n, [MarshalAs(UnmanagedType.LPArray, SizeParamIndex=0)] var[] v);

	[DllImport (DLL, CallingConvention=CallingConvention.Cdecl)]
	static extern
		void invoke_withstring(int n, [MarshalAs(UnmanagedType.LPArray, SizeParamIndex=0)] var[] v,
			int sn, [MarshalAs(UnmanagedType.LPArray, ArraySubType=UnmanagedType.LPStr, SizeParamIndex=2)] string[] vs);

	bool pushvalue(ref var v, object arg, ref string[] str) {
		if (arg == null) {
			v.type = (int)var_type.NIL;
		} else {
			Type t = arg.GetType();
			if (t == typeof(int)) {
				v.type = (int)var_type.INTEGER;
				v.d = (int)arg;
			} else if ( t == typeof(long)) {
				v.type = (int)var_type.INTEGER;
				v.d64 = (long)arg;
			} else if (t == typeof(float)) {
				v.type = (int)var_type.REAL;
				v.f = (float)arg;
			} else if (t == typeof(double)) {
				v.type = (int)var_type.REAL;
				v.f = (double)arg;
			} else if (t == typeof(bool)) {
				v.type = (int)var_type.BOOLEAN;
				v.d = (bool)arg ? 1 : 0;
			} else if (t == typeof(string)) {
				int idx = str.Length;
				Array.Resize(ref str, idx + 1);
				v.type = (int)var_type.STRING;
				v.d = idx;
				str[idx] = (string)arg;
			} else {
				return false;
			}
		}
		return true;
	}
	
	public void request(params object[] arg) {
		int n = arg.Length;
		Array.Resize(ref v, n);
		Array.Resize(ref vs, 0);
		for (int i = 0; i < n; i++) { 
			if (!pushvalue(ref v[i], arg[i], ref vs)) {
				throw new ArgumentException(String.Format("Unsupport type : {1} at {0}", i, arg[i].GetType()));
			}
		}
		int sn = vs.Length;
		if (sn == 0) {
			invoke(n, v);
		} else {
			invoke_withstring(n, v, sn, vs);
		}
	}
};

public class HelloWorld
{
	static public void Main() {
		Value v = new Value();
		v.request(1,2.0,"hello", false);
	}
}
