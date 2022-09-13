#include <iostream>
#include <string>
#include <fstream>
using namespace std;
int main(){
  fstream file;
  file.open("./log.txt",ios::binary|ios::in|ios::out|ios::trunc);
  string s="hello!";
  string t="yesss!";
  for(int i=0;i<1000;i++){
    s += "hello!";
    t += "yesss!";
  }
  file.seekp(6*5);
  file.write(s.c_str(),s.length());
//   //file.flush();
//   file.seekp(6*5);
//   char data[31];
//   data[31]='\0';
//   file.read(data,30);
//   printf("data1:%s\n",data);
//   file.seekp(6*5);
//   file.write(t.c_str(),t.length());
//   //file.flush();
//   file.seekp(6*5);
//   file.read(data,30);
//   printf("data2:%s\n",data);
}