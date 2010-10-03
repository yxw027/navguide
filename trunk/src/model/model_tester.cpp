#include "model.h"

void usage ()
{
    printf ("model_tester <filename>\n");
}

int main (int argc, char *argv[])
{
    if (argc < 2) {
        usage ();
        return 0;
    }

    const char *filename = argv[1];

    model_t *model = (model_t*)malloc(sizeof(model_t));

    model_init (model);

    model_read (model, filename);

    return 0;
}

