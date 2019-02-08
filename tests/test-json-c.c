
#include <stdio.h>

#include <json-c/json.h>




int main1() {
   int rc;
   char * s = "{ 'a': 77, 'x': 'a\0b'}";
   enum json_tokener_error jerr;

   struct json_object *jobj  = json_tokener_parse_verbose	 (s, &jerr);

   fprintf(stderr, "Error: %s\n", json_tokener_error_desc(jerr));
   s = json_object_to_json_string(jobj);
   printf ("s = %s\n", s);

   struct json_object *o;
   
   o = json_object_new_int (42);
   rc = json_object_object_add(jobj, "hgg", o);
   printf ("rc = %d\n", rc);

   o = json_object_new_string ("bar");
   rc = json_object_object_add(jobj, "foo\"", o);
   printf ("rc = %d\n", rc);

   s = json_object_to_json_string(jobj);
   printf ("s = %s\n", s);
}

int main() {
   enum json_tokener_error jerr;

   struct json_object *jobj = json_object_from_file ("test1.json");
   fprintf(stderr, "Error: %s\n", json_tokener_error_desc(jerr));

   const char *s = json_object_to_json_string(jobj);
   printf ("s = %s\n", s);
};

