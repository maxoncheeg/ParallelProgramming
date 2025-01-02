#include <cstdio>
#include "mpi.h"
#include <iostream>
#include <vector>

using namespace std;


double f(double x)
{
    return x * x * x;
}

static void first(int argc, char** argv)
{
    int rank, size;
    double dx = 1e-7;
    double start = 0, finish = 40;

    double local, global = 0;

    MPI_Init(&argc, &argv);
    clock_t start_t = clock();

    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    double a = (finish - start) / size * rank;
    double b = a + (finish - start) / size;
    double x = a;
    double sum = 0;
    while (x < b - dx)
    {
        sum += f((x + x + dx) / 2.0) * dx;
        x += dx;
    }
    cout << rank << ": " << sum << endl;

    local = sum;
    MPI_Allreduce(&local, &global, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);

    MPI_Finalize();
    if (rank == 0)
    {
        clock_t end_t = clock();
        cout << endl << "RESULT: " << global << endl;
        cout << endl << "TIME(ms): " << end_t - start_t << endl;
    }
}

enum
{
    N = 4,
    TEST = true,
    RANDOM_MAX = 10,
};

// 4 п - 6 р | 3
// 9 п - 6 р | 2
static int get_block_size(int size)
{
    int sqrt_size = sqrt(size);

    if (N % sqrt_size == 0)
        return sqrt(N * N / size);

    throw exception("Incorrect number of processes");
}


static void generate_block(int** A, int* B, int i, int j, int block_size, int matrix[N][N], int b[N])
{
    for (int k = 0; k < block_size; k++)
    {
        B[k] = b[k + j];
        A[k] = new int[block_size];
        for (int l = 0; l < block_size; l++)
            A[k][l] = matrix[k + i][j + l];
    }
}


static void second(int argc, char** argv)
{
    int rank, size;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // проверяем, что кол-во процессов можно использовать для разбиения на равные блоки
    int block_size = get_block_size(size);
    int block_in_row = N / block_size;

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    if (rank == 0)
    {
        cout << block_in_row << " " << block_size << endl;
    }
    

    // создаем производный тип для передачи массивов
    MPI_Datatype array_type;
    MPI_Type_contiguous(block_size, MPI_INT, &array_type);
    MPI_Type_commit(&array_type);


    if (rank == 0)
    {
        clock_t start = clock();
        int matrix[N][N];
        int vector[N];
        srand(time(NULL));

        // заполнение массива
        cout << "\tA | B\n";
        for (int i = 0; i < N; i++)
        {
            vector[i] = TEST ? 1 : rand() % RANDOM_MAX;

            for (int j = 0; j < N; j++)
            {
                matrix[i][j] = TEST ? j : rand() % RANDOM_MAX;
                cout << matrix[i][j] << "\t";
            }
            cout << "|\t" << vector[i] << "\n";
        }

        if (size == 1)
        {
            int result[N];
            for (int i = 0; i < N; ++i)
            {
                int sum = 0;
                for (int j = 0; j < N; ++j)
                    sum += matrix[i][j] * vector[j];
                result[i] = sum;
            }

            cout << "RESULT" << ": " << endl;

            for (int i : result)
                cout << i << "\t";

            cout << '\n';
            clock_t end = clock();
            cout << "TIME(ms): " << end - start << endl;
        }
        else
        {
            int send_rank = 1;
            int **base_a = nullptr, *base_b = nullptr;
            for (int i = 0; i < block_in_row; ++i)
                for (int j = 0; j < block_in_row; ++j)
                {
                    int index = i * block_size,
                        jindex = j * block_size,
                        **a = new int*[block_size],
                        *b = new int[block_size];

                    // заполняем массивы для передачи из изначального массива
                    generate_block(a, b, index, jindex, block_size, matrix, vector);

                    if (j == 0 && i == 0)
                    {
                        // процесс 0
                        base_a = a;
                        base_b = b;
                    }
                    else
                    {
                        // отправляем A построчно, и B

                        for (int k = 0; k < block_size; k++)
                            MPI_Send(a[k], 1, array_type, send_rank, i, MPI_COMM_WORLD);
                        MPI_Send(b, 1, array_type, send_rank, i, MPI_COMM_WORLD);
                        send_rank++;

                        for (int k = 0; k < block_size; k++)
                            delete[] a[k];
                        delete[] a;
                        delete[] b;
                    }
                }

            int* result = new int[block_size];
            for (int i = 0; i < block_size; ++i)
            {
                int sum = 0;
                for (int j = 0; j < block_size; ++j)
                    sum += base_a[i][j] * base_b[j];
                result[i] = sum;
            }
            for (int k = 0; k < block_size; k++)
                delete[] base_a[k];
            delete[] base_a;
            delete[] base_b;

            for (int j = 0; j < block_size; j++)
            {
                if (j == 0)
                    cout << rank << ": ";
                cout << result[j] << "\t";
            }
            cout << '\n';

            int* row_result = new int[block_size];

            for (int j = 0; j < block_in_row - 1; j++)
            {
                MPI_Recv(row_result, 1, array_type, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                for (int i = 0; i < block_size; ++i)
                {
                    result[i] += row_result[i];
                }
            }

            int result_vector[N];

            // for (int j = 0; j < block_size; j++)
            //     cout << rank << "m: " << result[j] << "\t";
            // cout << '\n';

            for (int j = 0; j < block_size; j++)
                result_vector[j] = result[j];
            delete[] result;

            // объединение данных из всех потоков
            for (int i = 0; i < block_in_row - 1; ++i)
            {
                MPI_Status status;
                MPI_Recv(row_result, 1, array_type, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
                int tag = status.MPI_TAG;
                for (int j = 0; j < block_size; j++)
                    result_vector[tag * block_size + j] = row_result[j];
            }
            delete[] row_result;

            cout << '\n';
            cout << "RESULT" << ": " << endl;

            for (int i : result_vector)
                cout << i << "\t";

            cout << '\n';
            clock_t end = clock();
            cout << "TIME(ms): " << end - start << endl;
            // MPI_Barrier(MPI_COMM_WORLD);
        }
    }
    else
    {
        int** a = new int*[block_size];
        int* b = new int[block_size];

        MPI_Status status;
        for (int i = 0; i < block_size; i++)
        {
            int* a_part = new int[block_size];
            MPI_Recv(a_part, 1, array_type, 0, MPI_ANY_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            a[i] = a_part;
        }
        MPI_Recv(b, 1, array_type, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
        int tag = status.MPI_TAG;

        int* result = new int[block_size];
        for (int i = 0; i < block_size; ++i)
        {
            int sum = 0;
            for (int j = 0; j < block_size; ++j)
                sum += a[i][j] * b[j];
            delete[] a[i];
            result[i] = sum;
        }
        delete[] a;
        delete[] b;

        int* row_result = new int[block_size];

        for (int j = 0; j < block_size; j++)
        {
            if (j == 0)
                cout << rank << ": ";
            cout << result[j] << "\t";
        }
        cout << '\n';



        if (rank == tag * block_in_row)
        {
            for (int j = 0; j < block_in_row - 1; j++)
            {
                MPI_Recv(row_result, 1, array_type, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                for (int i = 0; i < block_size; ++i)
                {
                    result[i] += row_result[i];
                }
            }

            // for (int j = 0; j < block_size; j++)
            //     cout << rank << "m: " << result[j] << "\t";
            // cout << '\n';


            // передаем с id расположения массива по порядку
            MPI_Send(result, 1, array_type, 0, rank / block_in_row, MPI_COMM_WORLD);
        }
        else
        {
            // for (int j = 0; j < block_size; j++)
            // {
            //     if (j == 0)
            //         cout << rank << ": ";
            //     cout << result[j] << "\t";
            // }
            // cout << '\n';


            MPI_Send(result, 1, array_type, tag * block_in_row, rank / block_in_row, MPI_COMM_WORLD);
        }

        delete[] result;
        delete[] row_result;
    }
    MPI_Type_free(&array_type);
    MPI_Finalize();
}

int main(int argc, char** argv)
{
    //first(argc, argv);
    second(argc, argv);

    return 0;
}
