#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#define DLLEXPORT

enum var_type {
	NIL = 0,
	INTEGER = 1,
	INT64 = 2,
	REAL = 3,
	BOOLEAN = 4,
	STRING = 5,
};

struct var {
	int type;
	int d;
	int64_t d64;
	double f;
};

DLLEXPORT void
invoke_withstring(int n, struct var *s, int sn, const char *str[]) {
	int i;
	for (i=0;i<n;i++) {
		struct var *v = &s[i];
		switch(s[i].type) {
		case NIL:
			printf("(nil)");
			break;
		case INTEGER:
			printf("%d", v->d);
			break;
		case INT64:
			printf("%d", (int)v->d64);
			break;
			break;
		case REAL:
			printf("%lf", v->f);
			break;
		case BOOLEAN:
			printf("%s", v->d ? "true" : "false");
			break;
		case STRING:
			printf("%s", str[v->d]);
			break;
		}
		printf(",");
	}
}

DLLEXPORT void
invoke(int n, struct var * s) {
	invoke_withstring(n,s,0, NULL);
}

