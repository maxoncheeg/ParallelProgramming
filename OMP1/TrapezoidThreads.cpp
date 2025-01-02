// OMP FIRST

#include <iostream>

using namespace std;

double function(double x)
{
    return x * x * x;
}

int main(int argc, char* argv[])
{
    double x0 = 0, xn = 20, dx = 1e-7f, result = 0;
    int threadAmount = 8;
    clock_t start_time, end_time, difference;

    cout << "\t\tINITIAL DATA:\n";
    cout << "\tX0: " << x0 << '\n';
    cout << "\tXn: " << xn << '\n';
    cout << "\tDx: " << dx << '\n';


    start_time = clock();
    const int steps = static_cast<int>((xn - x0) / dx);
    if (steps <= threadAmount) throw std::exception("steps <= threadAmount");

#pragma omp parallel for num_threads(threadAmount) reduction(+:result)
    for (int i = 0; i < steps; i++)
    {
        const double x = i * dx;
        if (x0 + x <= xn)
            result += function(x0 + x);
    }

    result = (result * 2 + function(x0) + function(xn)) * dx / 2;

    end_time = clock();
    difference = end_time - start_time;
    cout << "\n\t\tWITH " << threadAmount << " THREADS:\n";
    cout << "\tRESULT: " << result << '\n';
    cout << "\tTIME (ms): " << difference << '\n';

    _CrtDumpMemoryLeaks();
    return 0;
}


//OMP SECOND

#include <iostream>
#include <fstream>
#include <omp.h>
#include <string>

#define ull unsigned long long

using namespace std;

struct read_data
{
    ifstream* file = nullptr;
    bool is_end = false;
    ull* number = nullptr;
};

struct calc_data
{
    int index;
    bool* is_end;
    ull* number;
};


void make_number_file()
{
    srand(time(nullptr));
    ofstream output("KMLG-input.txt", std::ofstream::out | std::ofstream::trunc);

    for (int i = 0; i < 10000; i++)
    {
        long long num = abs(rand() * 1000000 + 1);

        output << num;

        if (i != 10000 - 1)
            output << endl;
    }

    output.close();
}


long long mul(long long a, long long b, long long m)
{
    if (b == 1)
        return a;
    if (b % 2 == 0)
    {
        long long t = mul(a, b / 2, m);
        return (2 * t) % m;
    }
    return (mul(a, b - 1, m) + a) % m;
}

long long pows(long long a, long long b, long long m)
{
    if (b == 0)
        return 1;
    if (b % 2 == 0)
    {
        long long t = pows(a, b / 2, m);
        return mul(t, t, m) % m;
    }
    return (mul(pows(a, b - 1, m), a, m)) % m;
}

long long gcd(long long a, long long b)
{
    if (b == 0)
        return a;
    return gcd(b, a % b);
}

bool ferma(long long x)
{
    if (x == 2)
        return true;
    srand(time(NULL));
    for (int i = 0; i < 100; i++)
    {
        long long a = (rand() % (x - 2)) + 2;
        if (gcd(a, x) != 1)
            return false;
        if (pows(a, x - 1, x) != 1)
            return false;
    }
    return true;
}

void* read(read_data* data)
{
    try
    {
        while (!data->file->eof())
        {
            if (*data->number == NULL)
            {
                string string_number;

#pragma omp critical
                {
                    getline(*data->file, string_number);
                }
                
                ull h = stoll(string_number);
                *data->number = h;
            }
        }

        data->is_end = true;
        return nullptr;
    }
    catch (exception e)
    {
        cout << "READ" << e.what() << endl;
        throw e;
        return NULL;
    }
}

void* calc(calc_data* data)
{
    try
    {
        string name = "KMLG-result-" + to_string(data->index) + ".txt";

        ofstream output(name);

        do
        {
            if (*data->number != NULL)
            {
                bool is_simple = ferma(*data->number);
                output << *data->number << " " << is_simple << endl;

                *data->number = NULL;
            }
        }
        while (!*data->is_end);

        output.close();
        return NULL;
    }
    catch (exception e)
    {
        cout << "CALC" << e.what() << endl;
        return NULL;
    }
}

int main(int argc, char* argv[])
{
    int thread_amount = 8;

    if (thread_amount % 2 != 0)
        return 0;

    int pair_amount = thread_amount / 2;

    make_number_file();
    cout << "File created!";
    ifstream input("KMLG-input.txt");

    ull* buffer = new ull[pair_amount];
    read_data* rdata = new read_data[pair_amount];
    calc_data* cdata = new calc_data[pair_amount];


    for (int i = 0; i < pair_amount; i++)
        buffer[i] = NULL;

    for (int i = 0; i < pair_amount * 2; i += 2)
    {
        const int index = i / 2;
        rdata[index].file = &input;

        cdata[index].index = index;
        cdata[index].number = rdata[index].number = &buffer[index];

        rdata[index].is_end = false;
        cdata[index].is_end = &rdata[index].is_end;
    }
    const clock_t start = clock();
#pragma omp parallel num_threads(thread_amount)
    {
        // читающие потоки
        int rank = omp_get_thread_num();
        if (rank % 2 == 0)
        {
            read(&rdata[rank / 2]);
        }
        else
        {
            calc(&cdata[rank / 2]);
        }
    }

    const clock_t end = clock();

    input.close();
    cout << endl << "Done in " << end - start << " ms!" << endl;
    cout << endl << "Press something cool to close this console..." << endl;
    getchar(); // NOLINT(cert-err33-c)

    delete[] buffer;
    delete[] rdata;
    delete[] cdata;

    _CrtDumpMemoryLeaks();

    return 0;
}
