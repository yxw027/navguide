#include <stdio.h>
#include <common/config.h> 


int
main (int argc, char ** argv)
{
    if (argc != 2) {
        fprintf (stderr, "Usage: %s filename\n", argv[0]);
        return 1;
    }

    char * filename = argv[1];
    FILE * f = fopen (filename, "r");
    if (!f) {
        fprintf (stderr, "Error opening %s\n", filename);
        return 1;
    }

    Config * conf = config_parse_file (f, filename);
    fclose (f);
    if (!conf)
        return 1;

    printf ("Dump of parse tree:\n");
    config_print (conf);

    double vald;
    int vali;
    char * str;
    double vals[4];
    char * strings[4];
    int n, i;

    printf ("\nSome configuration values:\n");

    if (config_get_double (conf, "sick.front.q", &vald) == 0)
        printf ("sick.front.q = %f\n", vald);

    if (config_get_double (conf, "sick.rear.q", &vald) == 0)
        printf ("sick.rear.q = %f\n", vald);

    if (config_get_boolean (conf, "xsens.calibrate_at_start", &vali) == 0)
        printf ("xsens.calibrate_at_start = %d\n", vali);

    if (config_get_str (conf, "sick.a", &str) == 0)
        printf ("sick.a = %s\n", str);

    if (config_get_int (conf, "sick.scans", &vali) == 0)
        printf ("sick.scans = %d\n", vali);

    n = config_get_double_array (conf, "sick.rear.d", vals, 4);
    if (n >= 0) {
        printf ("sick.rear.d = [ ");
        for (i = 0; i < n; i++)
            printf ("%f ", vals[i]);
        printf ("]\n");
    }

    n = config_get_str_array (conf, "sick.rear.b", strings, 4);
    if (n >= 0) {
        printf ("sick.rear.b = [ ");
        for (i = 0; i < n; i++)
            printf ("\"%s\" ", strings[i]);
        printf ("]\n");
    }

    printf ("(default) sick.bar = %d\n",
            config_get_int_or_default (conf, "sick.bar", 123));
    printf ("(default) sick.scans = %d\n",
            config_get_int_or_default (conf, "sick.scans", 123));

    config_free (conf);


    printf ("\nConfig parsing test complete.\n");
    conf = config_alloc ();

    int int_array[8];
    int bool_array[8];
    double double_array[8];
    for (n = 0; n < 8; ++n) {
      int_array[n] = 9-n;
      bool_array[n] = n % 2;
      double_array[n] = (double)n*n/(n+1);
    }

    config_set_int_array (conf, "first.second.third", int_array, 8);
    config_set_double (conf, "first.third", 3.14159);
    config_set_boolean (conf, "second", 1);
    config_set_boolean (conf, "second.third", 0);
    config_set_boolean_array (conf, "bools", bool_array, 8);
    config_set_double_array (conf, "doubles", double_array, 8);

    config_print (conf);
    printf ("\nConfig creation test complete.\n");

    config_free (conf);

    return 0;
}
