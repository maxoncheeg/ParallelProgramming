#pragma once

struct integrate_data
{
    double dx;
    double x0;
    double xn;
    int steps;
    double (*function)(double);
};