#include <string>
#include <cassert>
#include <cstdio>
#define F(x) x
#define LED_BUILTIN 13
#define HIGH 1
#define LOW 0
#define OUTPUT 1
static int led=-1;
void pinMode(int,int) {}
void digitalWrite(int,int value) {led=value;}
struct MockSerial {
  std::string input,output;
  void begin(int) {}
  int available() {return (int)input.size();}
  int read() {unsigned char c=input[0];input.erase(0,1);return c;}
  void print(const char* s) {output+=s;}
  void print(char c) {output+=c;}
  void println() {output+='\n';}
  void println(const char* s) {print(s);println();}
} Serial;
#include "../tiny_uno/tiny_uno.ino"
std::string run(const std::string& s) {Serial.output.clear();Serial.input=s;loop();return Serial.output;}
int main() {
  setup();assert(Serial.output.find("READY\n")!=std::string::npos);
  auto hello=run("hello\n"); assert(hello.find("TEXT hello") == 0);assert(hello.find("READY\n")!=std::string::npos);assert(led==LOW);
  assert(run("hello\r\n")==hello);
  assert(run(std::string(63,'a')+"\n").find("ERROR")==std::string::npos);
  assert(run(std::string(64,'a')+"\n")=="ERROR prompt too long; max 63 bytes\nREADY\n");
  assert(run("hello\n")==hello);
  assert(run("HELLO\n")==hello);
  assert(run("!!!\n").find("TEXT    ")==0);
  assert(run("\n").find("TEXT ")==0);assert(led==LOW);
  assert(run("hel").empty());assert(run("lo\n")==hello);
  puts("PASS: actual sketch mock, limits 63/64, overflow recovery, CRLF, blank/invalid/uppercase input, partial lines, LED LOW");
}
