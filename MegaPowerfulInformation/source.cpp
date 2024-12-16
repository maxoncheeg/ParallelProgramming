#include <cstdio>
#include "mpi.h"
#include <iostream>
#include <vector>

enum
{
    N = 4,
    isOnes = false
};

using namespace std;

static double f(double x)
{
    return x * x * x;
}

struct Block
{
public:
    int a[N][N];
    int b[N];
    int n;

    Block()
    = default;
};

static int get_block_size(int size)
{
    for (int i = 1; i <= N; i++)
        if (N % i == 0 && size == i * i)
            return i;

    throw new exception("Incorrect number of proccesses");
}

static Block generate_block(int i, int j, int block_size, int matrix[N][N], int b[N])
{
    Block block;

    block.n = block_size;

    for (int k = 0; k < block_size; k++)
    {
        block.b[k] = (b[k + i]);
        for (int l = 0; l < block_size; l++)
            block.a[k][l] = (matrix[k + i][j + l]);
    }

    return block;
}

void create_mpi_datatype(MPI_Datatype* datatype)
{
    Block block;
    MPI_Datatype types[3] = {MPI_INT, MPI_INT, MPI_INT};
    int block_lengths[3] = {N * N, N, 1};
    MPI_Aint displacements[3];
    MPI_Aint addresses[4];

    MPI_Get_address(&block, &addresses[0]);
    MPI_Get_address(&(block.a), &addresses[1]);
    MPI_Get_address(&(block.b), &addresses[2]);
    MPI_Get_address(&(block.n), &addresses[3]);

    displacements[0] = addresses[1] - addresses[0];
    displacements[1] = addresses[2] - addresses[0];
    displacements[2] = addresses[3] - addresses[0];

    MPI_Type_create_struct(3, block_lengths, displacements, types, datatype);
    MPI_Type_commit(datatype);
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
    // Блочное разделение данных.
    // Исходные данные первоначально размещаются в одном процессе,
    // в двумерном и одномерном массиве соответственно.
    // Рассылка исходных данных осуществляется посредством парных взаимодействий
    // с использованием производных типов данных. Результат собирается в один процесс.
    // Сборка результатов при помощи парных взаимодействий и производных типов.
    // Виртуальные топологии не используются.


    int rank, size;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    int block_size = get_block_size(size);

    MPI_Datatype block_type;
    create_mpi_datatype(&block_type);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    Block data;
    if (rank == 0)
    {
        int matrix[N][N];
        int b[N];
        srand(time(NULL)); // NOLINT(clang-diagnostic-shorten-64-to-32, cert-msc51-cpp)

        cout << "A: \n";
        for (auto& i : matrix)
        {
            for (int& j : i)
            {
                j = isOnes ? 1 : rand() % 20;
                cout << j << "\t";
            }

            cout << '\n';
        }

        cout << "B: \n";
        for (auto& i : b)
        {
            i = isOnes ? 1 : rand() % 20;
            cout << i << "\n";
        }


        int block_in_row = N / block_size;

        cout << "Blocks in row: " << block_in_row << endl;

        int size_counter = 1;
        for (int i = 0; i < block_in_row; ++i)
        {
            vector<Block*> row_blocks; // roblox

            for (int j = 0; j < block_in_row; ++j)
            {
                int index = i * block_size, jindex = j * block_size;

                Block new_block = generate_block(index, jindex, block_size, matrix, b);

                if (j == 0 && i == 0)
                {
                    data = new_block;
                }
                else
                {
                    MPI_Send(&new_block, 1, block_type, size_counter++, j, MPI_COMM_WORLD);
                }
            }
            
            cout << "blocks for " << i << " row was configured\n";

            //MPI_Barrier(MPI_COMM_WORLD);
                //getchar();
        }
    }
    else
    {
        MPI_Status status;
        MPI_Recv(&data, 1, block_type, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
        int tag = status.MPI_TAG;

        cout << "block received from " << status.MPI_TAG << " in rank " << rank << "\n";
        cout << "recv: " << data.n << endl;
        //cout << "a: " << nblock.a[0][0] << endl;

        for (int i = 0; i < data.n; ++i)
        {
            for (int j = 0; j < data.n; ++j)
            {
                cout << data.a[i][j] << "\t";
            }
            cout << '\n';
        }
        cout << '\n';

        MPI_Allreduce()
        

        //MPI_Barrier(MPI_COMM_WORLD);
    }
    
    MPI_Finalize();
}

int main(int argc, char** argv)
{
    second(argc, argv);

    return 0;
}
