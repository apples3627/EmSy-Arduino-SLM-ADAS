#pragma once
#include <stdint.h>
#include <stddef.h>
namespace CommandPlan {
static const size_t MAX_BYTES=192, MAX_CLAUSES=4, CLAUSE_BYTES=96;
enum Reason {OK=0,BAD_INPUT=1,TOO_LONG=2,TOO_MANY=3,EMPTY_CLAUSE=4,CLAUSE_TOO_LONG=5,BAD_CONNECTOR=6,INFERENCE_REJECTED=7,CONFLICT=8};
struct Split {uint8_t start[4],length[4],count;Reason reason;};
inline char lower(char c){return c>='A'&&c<='Z'?c+32:c;}
inline bool word(const char*t,size_t n,size_t i,const char*w,size_t k){if(i+k>n||(i&&t[i-1]!=' '))return false;for(size_t j=0;j<k;j++)if(lower(t[i+j])!=w[j])return false;return i+k==n||t[i+k]==' ';}
inline bool push(const char*t,size_t a,size_t b,Split&o){while(a<b&&t[a]==' ')a++;while(b>a&&t[b-1]==' ')b--;if(a==b){o.reason=EMPTY_CLAUSE;return false;}if(b-a>CLAUSE_BYTES){o.reason=CLAUSE_TOO_LONG;return false;}if(o.count==MAX_CLAUSES){o.reason=TOO_MANY;return false;}o.start[o.count]=a;o.length[o.count++]=b-a;return true;}
// Exactly one connector per boundary; comma followed by and/also/plus and 'and also' accepted.
inline bool split(const char*t,size_t n,Split&o){o.count=0;o.reason=OK;if(!t||!n){o.reason=BAD_INPUT;return false;}if(n>MAX_BYTES){o.reason=TOO_LONG;return false;}for(size_t i=0;i<n;i++)if((unsigned char)t[i]<32||(unsigned char)t[i]>126){o.reason=BAD_INPUT;return false;}
 size_t a=0;for(size_t i=0;i<n;){size_t k=0;if(t[i]==','||t[i]==';')k=1;else if(word(t,n,i,"and",3))k=3;else if(word(t,n,i,"also",4)||word(t,n,i,"plus",4))k=4;if(!k){i++;continue;}if(!push(t,a,i,o))return false;bool punct=t[i]==','||t[i]==';',wasAnd=k==3;i+=k;while(i<n&&t[i]==' ')i++;if(punct){if(word(t,n,i,"and",3)){i+=3;wasAnd=true;}else if(word(t,n,i,"also",4)||word(t,n,i,"plus",4))i+=4;while(i<n&&t[i]==' ')i++;}if(wasAnd&&word(t,n,i,"also",4)){i+=4;while(i<n&&t[i]==' ')i++;}a=i;}return push(t,a,n,o);}
inline bool extract(const char*t,const Split&s,uint8_t i,char*out){if(i>=s.count)return false;size_t a=s.start[i],b=a+s.length[i];while(a<b&&t[a]==' ')a++;while(b>a&&t[b-1]==' ')b--;while(b>a&&(t[b-1]=='.'||t[b-1]=='!'||t[b-1]=='?'))b--;while(b>a&&t[b-1]==' ')b--;size_t j=0;for(;a<b;a++)out[j++]=lower(t[a]);out[j]=0;return j>0;}
struct Decisions {uint8_t intents[4],count;bool stop,rejected;Reason reason;void reset(){count=0;stop=rejected=false;reason=OK;}bool append(uint8_t v){if(v==2){stop=true;count=1;intents[0]=2;return true;}if(stop)return true;if(v==7||v>9){rejected=true;reason=INFERENCE_REJECTED;return false;}for(uint8_t i=0;i<count;i++){uint8_t u=intents[i];if(u==v)return true;if((u==0&&v==1)||(u==1&&v==0)||(u==3&&v==4)||(u==4&&v==3)||(u==5&&v==6)||(u==6&&v==5)||(u==8&&v==9)||(u==9&&v==8)){rejected=true;if(reason!=INFERENCE_REJECTED)reason=CONFLICT;}}if(count<4)intents[count++]=v;else{rejected=true;reason=TOO_MANY;}return !rejected;}bool finish(){if(stop){rejected=false;reason=OK;count=1;intents[0]=2;return true;}if(rejected||!count){count=0;if(reason==OK)reason=INFERENCE_REJECTED;return false;}return true;}};
}
