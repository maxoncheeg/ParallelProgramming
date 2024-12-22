#include <cstdio>
#include "mpi.h"
#include <iostream>
#include <vector>

using namespace std;

enum
{
    N = 10,
    TEST = true,
    RANDOM_MAX = 10,
};

double f(double x)
{
    return x * x * x;
}

// 4 п - 6 р | 3
// 9 п - 6 р | 2
static int get_block_size(int size)
{
    int sqrt_size = sqrt(size);

    if (N % sqrt_size == 0)
        return sqrt(N * N / size);

    throw new exception("Incorrect number of proccesses");
}

static void generate_block(int** A, int* B, int i, int j, int block_size, int matrix[N][N], int b[N])
{
    for (int k = 0; k < block_size; k++)
    {
        B[k] = b[k + i];
        A[k] = new int[block_size];
        for (int l = 0; l < block_size; l++)
            A[k][l] = matrix[k + i][j + l];
    }
}

void sum_operation(int* invec, int* inoutvec, int* len, MPI_Datatype* dtype)
{
    for (int i = 0; i < *len; i++)
    {
       // cout << " " << i << "|" << invec[i];
        inoutvec[i] += invec[i];
    }
    //cout << " - " << *len << endl;
}

static void first(int argc, char** argv)
{
    int rank, size;
    double dx = 1e-5;
    double start = 0, finish = 20;

    double local, global = 0;

    MPI_Init(&argc, &argv);

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
        cout << endl << "RESULT: " << global << endl;
}

static void second(int argc, char** argv)
{
    int rank, size;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // проверяем, что кол-во процессов можно использовать для разбиения на равные блоки
    int block_size = get_block_size(size);
    int block_in_row = N / block_size;

    // block_size = 2;
    //  block_in_row = 3;

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    if (rank == 0)
    {
        cout << block_in_row << " " << block_size << endl;
    }

    MPI_Op array_sum;
    MPI_Op_create((MPI_User_function*)sum_operation, block_size, &array_sum);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    // создаем производный тип для передачи массивов
    MPI_Datatype array_type;
    MPI_Type_contiguous(block_size, MPI_INT, &array_type);
    MPI_Type_commit(&array_type);


    // создание парных коммуникаторов
    MPI_Group* groups = new MPI_Group[block_in_row];
    MPI_Comm* comms = new MPI_Comm[block_in_row];
    MPI_Group base_group;
    MPI_Comm_group(MPI_COMM_WORLD, &base_group);

    int* ranks = new int[block_in_row];

    for (int i = 0, r = 0; i < block_in_row; i++)
    {
        for (int j = 0; j < block_in_row; j++)
        {
            ranks[j] = r;
            r++;
        }

        MPI_Group_incl(base_group, block_in_row, ranks, &groups[i]);
        MPI_Comm_create(MPI_COMM_WORLD, groups[i], &comms[i]);
    }
    delete[] ranks;
    //delete[] ranks;
    int* row_result = new int[block_size + 1];

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

        MPI_Allreduce(result, row_result, block_size, array_type, array_sum, comms[0]);
        delete[] result;

        int result_vector[N];

        for (int j = 0; j < block_size; j++)
            result_vector[j] = row_result[j];


        // объединение данных из всех потоков
        for (int i = 0; i < block_in_row - 1; ++i)
        {
            MPI_Status status;
            MPI_Recv(row_result, 1, array_type, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
            int tag = status.MPI_TAG;
            for (int j = 0; j < block_size; j++)
                result_vector[tag * block_size + j] = row_result[j];
        }

        cout << '\n';
        cout << "RESULT" << ": " << endl;

        for (int i : result_vector)
            cout << i << "\t";

        cout << '\n';
        clock_t end = clock();
        cout << "TIME(ms): " << end - start << endl;
        // MPI_Barrier(MPI_COMM_WORLD);
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


        MPI_Allreduce(result, row_result, block_size, array_type, array_sum, comms[tag]);

        if (rank == tag * block_in_row)
        {
            // передаем с id расположения массива по порядку
            MPI_Send(row_result, 1, array_type, 0, rank / block_in_row, MPI_COMM_WORLD);
        }

        delete [] result;
    }

    MPI_Barrier(MPI_COMM_WORLD);
    //delete[] row_result;

    
    for (int i = 0; i < block_in_row; ++i)
    {
        //cout << i;
        MPI_Group_free(&groups[i]);
        //MPI_Comm_free(&comms[i]);
    }
    MPI_Op_free(&array_sum);
    MPI_Type_free(&array_type);


    MPI_Finalize();
}

int main(int argc, char** argv)
{
    second(argc, argv);

    return 0;
}
