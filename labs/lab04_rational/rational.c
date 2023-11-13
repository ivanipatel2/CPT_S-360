#include <stdlib.h>
#include <stdio.h> // for printf()
#include "rational.h"


static Rational rtnl_simplify(Rational rtnl) {
  int a,b,tmp;
  a=abs(rtnl.num);
  b=abs(rtnl.denom);
  while (b!=0) {
    tmp=b;
    b=a%b;
    a=tmp;
  }
  rtnl.num/=a;
  rtnl.denom/=a;
  if (rtnl.denom<0) {
    rtnl.num *= -1;
    rtnl.denom *= -1;
  }
  return rtnl;
}

Rational rtnl_add(Rational rtnl0, Rational rtnl1) {
  Rational r;
  r.num = rtnl0.num*rtnl1.denom + rtnl1.num*rtnl0.denom;
  r.denom = rtnl0.denom*rtnl1.denom;
  return rtnl_simplify(r);
}

Rational rtnl_sub(Rational rtnl0, Rational rtnl1) {
  Rational r;
  r.num = rtnl0.num*rtnl1.denom - rtnl1.num*rtnl0.denom;
  r.denom = rtnl0.denom*rtnl1.denom;
  return rtnl_simplify(r);
}

Rational rtnl_mul(Rational rtnl0, Rational rtnl1) {
  Rational r;
  r.num = rtnl0.num*rtnl1.num;
  r.denom = rtnl0.denom*rtnl1.denom;
  return rtnl_simplify(r);
}

Rational rtnl_div(Rational rtnl0, Rational rtnl1) {
  Rational r;
  r.num = rtnl0.num*rtnl1.denom;
  r.denom = rtnl0.denom*rtnl1.num;
  return rtnl_simplify(r);
}

Rational rtnl_init(int num, int denom) {
  Rational r;
  r.num = num;
  r.denom = denom;
  return rtnl_simplify(r);
}

Rational rtnl_ipow(Rational rtnl, int ipow) {
  int i;
  Rational r;
  r.num=1;
  r.denom=1;
  if (ipow < 0) {
	i = rtnl.num;
	rtnl.num = rtnl.denom;
	rtnl.denom = i;
    ipow *= -1;
  }
  for (i=0;i<ipow;i++)
    r = rtnl_mul(r,rtnl);
  return r;
}

char *rtnl_asStr(Rational rtnl, char buf[RTNL_BUF_SIZE]) {
    snprintf(buf, RTNL_BUF_SIZE,"(%d/%d)",rtnl.num,rtnl.denom);
    return buf;
}
