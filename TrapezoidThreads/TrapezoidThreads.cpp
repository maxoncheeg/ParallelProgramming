#include <iostream>
#include "trapezoid/TrapezoidIntegrateService.h"

using namespace std;

double function(double x)
{
    return x * x * x;
}

int main(int argc, char* argv[])
{
    trapezoid_integrate_service service = trapezoid_integrate_service(function);

    double x0 = 0, xn = 20, dx = 1e-7f, result;
    int threadAmount = 4;
    clock_t start_time, end_time, difference;

    cout << "\t\tINITIAL DATA:\n";
    cout << "\tX0: " << x0 << '\n';
    cout << "\tXn: " << xn << '\n';
    cout << "\tDx: " << dx << '\n';

    start_time = clock();
    result = service.integrate_trapezoid(dx, x0, xn);
    end_time = clock();
    difference = end_time - start_time;
    cout << "\n\t\tWITHOUT THREADS:\n";
    cout << "\tRESULT: " << result << '\n';
    cout << "\tTIME (ms): " << difference << '\n';

    start_time = clock();
    result = service.integrate_trapezoid(dx, x0, xn, threadAmount);
    end_time = clock();
    difference = end_time - start_time;
    cout << "\n\t\tWITH " << threadAmount << " THREADS:\n";
    cout << "\tRESULT: " << result << '\n';
    cout << "\tTIME (ms): " << difference << '\n';

    return 0;
}
