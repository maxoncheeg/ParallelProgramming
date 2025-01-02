
#include <iostream>
#include <fstream>
#include <string>

#define ull unsigned long long

using namespace std;

#include "pthread/pthread.h"

#pragma comment(lib, "pthread/pthreadVCE2.lib")

pthread_mutex_t mutex;

struct read_data
{
    ifstream* file = nullptr; // Указатель на поток входных данных
    bool is_end = false; // Флаг "Каретка в конце файла"
    ull* number = nullptr; // Указатель на число
};

struct calc_data
{
    int index; // Индекс пары потоков
    bool* is_end; // Указатель на флаг структуры чтения
    ull* number; // Указатель на число
};

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

void* read(void* x)
{
    try
    {
        read_data* data = static_cast<read_data*>(x);


        while (!data->file->eof())
        {
            if (*data->number == NULL)
            {
                string string_number;

                pthread_mutex_lock(&mutex);
                getline(*data->file, string_number);
                pthread_mutex_unlock(&mutex);
                
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

void* calc(void* x)
{
    try
    {
        calc_data* data = static_cast<calc_data*>(x);

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

int main(int argc, char* argv[])
{
    pthread_mutex_init(&mutex, nullptr);


    make_number_file();
    cout << "File created!";
    ifstream input("KMLG-input.txt");
    int pair_amount = 2;


    pthread_t* ids = new pthread_t[pair_amount * 2];
    ull* buffer = new ull[pair_amount];
    read_data* rdata = new read_data[pair_amount];
    calc_data* cdata = new calc_data[pair_amount];


    for (int i = 0; i < pair_amount; i++)
        buffer[i] = NULL;

    const clock_t start = clock();
    
    for (int i = 0; i < pair_amount * 2; i += 2)
    {
        const int index = i/2;
        rdata[index].file = &input;
        
        cdata[index].index = index;
        cdata[index].number = rdata[index].number = &buffer[index];
        
        rdata[index].is_end = false;
        cdata[index].is_end = &rdata[index].is_end;

        pthread_create(&ids[i], nullptr, read, &rdata[index]);
        pthread_create(&ids[i + 1], nullptr, calc, &cdata[index]);
    }


    for (int i = 0; i < pair_amount * 2; i += 2) // ожидаем читающие потоки
        pthread_join(ids[i], nullptr);
    for (int i = 1; i < pair_amount * 2; i += 2) // ожидаем считающие потоки
        pthread_join(ids[i], nullptr);

    const clock_t end = clock();

    input.close();
    cout << endl << "Done in " << end - start << " ms!" << endl;
    cout << endl << "Press something cool to close this console..." << endl;
    getchar();  // NOLINT(cert-err33-c)
    
    delete[] ids;
    delete[] buffer;
    delete[] rdata;
    delete[] cdata;

    _CrtDumpMemoryLeaks();

    return 0;
}
