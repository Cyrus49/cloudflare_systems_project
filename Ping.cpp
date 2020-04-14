#include <cassert>
#include <iostream>
using namespace std;

class Ping{
  Ping(){

  }
};
int main(int argc, char *argv[]){
    assert(argc>1);
    cout<<argv[1]<<endl;
}
