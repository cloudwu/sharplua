#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#define DLLEXPORT

enum var_type {
	NIL = 0,
	INTEGER = 1,
	REAL = 2,
	BOOLEAN = 3,
	STRING = 4,
};

struct var {
	enum var_type type;
	int64_t d;
	double f;
	bool b;
	const char *s;
};

DLLEXPORT void
invoke(int n, struct var * s) {
	int i;
	for (i=0;i<n;i++) {
		struct var *v = &s[i];
		switch(s[i].type) {
		case NIL:
			printf("(nil)");
			break;
		case INTEGER:
			printf("%d", (int)v->d);
			break;
		case REAL:
			printf("%lf", v->f);
			break;
		case BOOLEAN:
			printf("%s", v->b ? "true" : "false");
			break;
		case STRING:
			printf("%s", v->s);
			break;
		}
		printf(",");
	}
}

