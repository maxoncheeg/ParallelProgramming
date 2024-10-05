#pragma once

class trapezoid_integrate_service
{
    double (*_function)(double);
    static void* integrate_trapezoid_thread_part(void* param);
public:
    trapezoid_integrate_service(double (*_function)(double));
    double integrate_trapezoid(double dx, double x0, double xn) const;
    double integrate_trapezoid(double dx, double x0, double xn, int threadAmount) const;
};
