#include "TrapezoidIntegrateService.h"
#include <iostream>

#include "trapezoid_integrate_data.h"
#include "pthread/pthread.h"

#pragma comment(lib, "trapezoid/pthread/pthreadVCE2.lib");

trapezoid_integrate_service::trapezoid_integrate_service(double (*_function)(double))
{
    this->_function = _function;
}

double trapezoid_integrate_service::integrate_trapezoid(double dx, double x0, double xn) const
{
    double sum = 0;
    const int steps = static_cast<int>(ceil((xn - x0) / dx));
    
    for (int i = 1; i < steps - 1; i++)
    {
        const double x = x0 + dx * i;
        if(x < xn)
            sum += _function(x);
    }
    
    return (sum * 2 + _function(x0) + _function(xn)) * dx / 2;
}

double trapezoid_integrate_service::integrate_trapezoid(double dx, double x0, double xn, int threadAmount) const
{
    if (threadAmount <= 0) throw std::invalid_argument("thread amount <= 0");
    if (x0 >= xn) throw std::invalid_argument("x0 >= xn");
    if (dx <= 0) throw std::invalid_argument("dx > 0");

    if (threadAmount == 1)
        return integrate_trapezoid(dx, x0, xn);

    const int steps = static_cast<int>((xn - x0) / dx);
    if (steps <= threadAmount) throw std::exception("steps <= threadAmount");
    const int stepsPerThread = static_cast<int>(ceil((steps) / threadAmount));

    // создаем id для всех потоков, которыми будем пользоваться
    pthread_t* ids = new pthread_t[threadAmount]; 
    
    pthread_attr_t attr; // узнать зачем оно
    pthread_attr_init(&attr);

    integrate_data data;
    data.x0 = x0;
    data.xn = xn;
    data.dx = dx;
    data.steps = stepsPerThread;
    data.function = _function;

    for (int i = 0; i < threadAmount; i++)
    {
        trapezoid_integrate_data* current_data = new trapezoid_integrate_data();
        current_data->data = &data;
        current_data->index = i;
 
        pthread_create(&ids[i], &attr, integrate_trapezoid_thread_part, current_data);
    }

    double sum = 0;
    for (int i = 0; i < threadAmount; i++)
    {
        double* threadResult = nullptr;
        pthread_join(ids[i], reinterpret_cast<void**>(&threadResult));
        
        sum += *threadResult;
        delete threadResult;
    }

    delete[] ids;
    return (sum * 2 + _function(x0) + _function(xn)) * dx / 2;
}

void* trapezoid_integrate_service::integrate_trapezoid_thread_part(void* param)
{
    const trapezoid_integrate_data* current_data = static_cast<trapezoid_integrate_data*>(param);
    const integrate_data data = *current_data->data;

    double* sum = new double();
    *sum = 0;
    
    for (int i = 0; i < data.steps; i++)
    {
        const double x = (i + current_data->index * data.steps) * data.dx;
        if (data.x0 + x <= data.xn)
            *sum += data.function(data.x0 + x);
    }
    
    delete current_data;
    return sum;
}
