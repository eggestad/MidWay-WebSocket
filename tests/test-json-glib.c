
#include<stdio.h>

#include <json-glib/json-glib.h>





int test(char * str) {


   JsonParser *parser =  json_parser_new ();
   GError *err = NULL;

   gboolean b;

   b = json_parser_load_from_data(parser, str, -1, &err);
   printf("b=%d\n", b);
   if (!b)
      printf("b=%d %s\n", b, err->message);

   JsonNode *root = json_parser_get_root (parser);

   JsonObject *obj =  json_node_get_object (root);

   JsonNode * newnode = json_node_alloc ();
   json_node_init_int(newnode, 88);
   json_object_set_member (obj, "xint", newnode);

   
   JsonGenerator *gen = json_generator_new ();
   json_generator_set_root(gen, root);

   
   gsize length;
  
   gchar * s =  json_generator_to_data(gen, &length);

   printf("s='%s' %d \n", s, length);
}

int main() {

   test("[ 1,2,{} ]");
   test("{ 'a': 5}");
}
