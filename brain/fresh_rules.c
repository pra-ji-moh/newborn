/*
 * fresh_rules.c -- held-out boxes: six rules never used while ob_novel.c was
 * developed, met by the three-year life that test_novel keeps on file.
 *   fresh_rules SNAPSHOT   (SNAPSHOT from: test_novel SNAPSHOT q)
 */
#include <stdio.h>
#include <string.h>
#include "ob_novel.h"
static ob_one_t S, O; static ob_world_t SW, W;
static unsigned pc(unsigned x){unsigned n=0;for(;x;x&=x-1)n++;return n;}
#define A(x) (1u<<(x))
static int r1(const of_test_t*t){return t->place==3u&&(t->acts&A(2));}
static int r2(const of_test_t*t){return t->place==1u&&(t->acts&(A(0)|A(3)))==(A(0)|A(3));}
static int r3(const of_test_t*t){return t->place==0u&&(t->acts&(A(1)|A(2)));}
static int r4(const of_test_t*t){return t->place==2u&&pc(t->acts&(A(0)|A(2)|A(3)))>=2u;}
static int r5(const of_test_t*t){return (t->acts&A(1))!=0u;}
static int r6(const of_test_t*t){return (t->acts&(A(3)|A(5)))==(A(3)|A(5));}
int main(int c,char**v){FILE*f=fopen(v[1],"rb"); if(fread(&S,sizeof S,1,f)!=1||fread(&SW,sizeof SW,1,f)!=1)return 1; ob_one_resume(&S);
 on_rule_t R[6]={r1,r2,r3,r4,r5,r6}; const char*N[6]={"act 2, q3","acts 0 and 3, q1","act 1 or 2, q0","two of 0,2,3, q2","act 1 anywhere","acts 3 and 5 anywhere"};
 for(int k=0;k<6;k++){on_meeting_t m; memcpy(&O,&S,sizeof O); memcpy(&W,&SW,sizeof W); on_box_meet(&W,&O,R[k],3000,0,101u+k,&m);
  printf("%-24s %s tests %u%s\n",N[k],m.found?"found    ":(m.tries?"WRONG    ":"not found"),m.tries, m.tries&&!m.found?" (claimed wrongly)":""); fflush(stdout);}
 return 0;}
