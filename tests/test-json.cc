#include <stdio.h>
#include <iostream>


#include <json/json.h>

using namespace Json;
using namespace std;

int main(int argc, char ** argv) {
   printf("start\n");

   Value v = new Value(66);

   std::cout << v;
   std::cout << std::endl;
   printf("start %s\n", v.asString().c_str() );
      
      
   Value a = Value( new StaticString("some text") );
   Value object;
   static const StaticString code("code");
   object[code] = 1234;
   std::cout << a;
   std::cout << std::endl;


   CharReaderBuilder builder;

   CharReader r = builder.newCharReader();

    
   r.parse("{ a: 99}", a);

   cout << r;
}

