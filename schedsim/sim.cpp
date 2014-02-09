
#include "sim.h"

void operator >>(Task &a, Task &b)
{
	b.addDep(&a);
}

void operator <<(Task &a, Task &b)
{
	a.addDep(&b);
}

ostream &operator <<(ostream &str, Task &t)
{
	return str << t.name;
}

