#include <stdio.h>
#include <math.h>

#include <common/car_model.h>


int
main (int argc, char ** argv)
{
    CarParams * ford;
    CarState s;
    CarInputs in;
    int i;

    ford = car_model_new_ford_escape ();

    car_model_init_state (&s, ford);

    in.gas_mv = 2500;
    in.steer_mv = 2400;
    for (i = 0; i < 60; i++) {
        car_model_step (&s, &in, 0.05);
        printf ("rpm: %f, shaft: %f\n", s.eng_speed * 60 / 2 / M_PI,
                s.shaft_speed * 60 / 2 / M_PI);
    }

    in.gas_mv = 3400;
    in.steer_mv = 2400;
    printf ("hitting gas\n");
    for (i = 0; i < 120; i++) {
        car_model_step (&s, &in, 0.05);
        printf ("rpm: %f, shaft: %f\n", s.eng_speed * 60 / 2 / M_PI,
                s.shaft_speed * 60 / 2 / M_PI);
    }

    in.gas_mv = 2500;
    in.steer_mv = 2400;
    printf ("cruising\n");
    for (i = 0; i < 120; i++) {
        car_model_step (&s, &in, 0.05);
        printf ("rpm: %f, shaft: %f\n", s.eng_speed * 60 / 2 / M_PI,
                s.shaft_speed * 60 / 2 / M_PI);
    }

    in.gas_mv = 1000;
    in.steer_mv = 2400;
    printf ("hitting brake\n");
    for (i = 0; i < 60; i++) {
        car_model_step (&s, &in, 0.05);
        printf ("rpm: %f, shaft: %f\n", s.eng_speed * 60 / 2 / M_PI,
                s.shaft_speed * 60 / 2 / M_PI);
    }

    car_model_free_params (ford);
    return 0;
}
