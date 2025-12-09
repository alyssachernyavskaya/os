#include <windows.h>  

// структура для потока
typedef struct 
{
    SIZE_T thread_id;        
    SIZE_T numrounds;// количество раундов на поток
    LONGLONG success;// счетчик успешных вариантов для каждого потока
    DWORD rand;// число для рандомайзера
} ThreadArgs;

static LONGLONG total = 0;// счетчик успешных вариантов суммарно для всех потоков
static HANDLE mutex;

//функция для вывода в консоль строки
void strprint(const char* str) 
{
    HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE);  
    if (hStdout == INVALID_HANDLE_VALUE) 
    {
        return;
    }
    DWORD written;                                      
    WriteFile(hStdout, str, lstrlenA(str), &written, NULL);  
}

// функция вывода числа в консоль
void numprint(SIZE_T num) 
{
    char buffer[32];  
    int pos = 31;     
    buffer[31] = '\0'; 
    if (num == 0) 
    {
        buffer[--pos] = '0';
    }
    else 
    {
        while (num > 0) 
        {
            buffer[--pos] = '0' + (num % 10);
            num /= 10;
        }
    }
    strprint(&buffer[pos]);
}

// преобразование строки в число
SIZE_T strtoint(const char* str) 
{
    SIZE_T result = 0; 
    while (*str >= '0' && *str <= '9') 
    {
        result = result * 10 + (*str - '0');
        str++;                               
    }
    return result;  
}

// вывод процентов
void printchance(double prob) 
{
    int percent = (int)(prob * 100);//целое
    int decimals = (int)(prob * 10000) % 100;//дробь
    strprint("chance: ");
    numprint(percent);
    strprint(".");
    if (decimals < 10) 
        strprint("0");
    numprint(decimals);
    strprint("%");
    strprint("\n");
}

//генератор рандомных чисел для генерации карточек
DWORD random(DWORD* rand)
{
    return (*rand = *rand * 123 + 456);
}

// проверка 2х верхних карт
DWORD cards(DWORD* rand)
{
    DWORD card1 = random(rand) % 13;
    DWORD card2 = random(rand) % 13;
    if (card1 == card2)
        return 1;
    else
        return 0;
}

// последовательный алгоритм!!!! - здесь нет потоков и все выполняется сразу
void one(SIZE_T rounds)
{
    DWORD rand = GetTickCount();
    LONGLONG well = 0;//счетчик успешных успехов
    DWORD start = GetTickCount();
    // Основной цикл - обрабатываем все раунды в одном потоке
    for (SIZE_T i = 0; i < rounds; i++)
    {
        well += cards(&rand);
    }
    DWORD time = GetTickCount() - start;
    strprint("long algorithm: ");
    numprint((SIZE_T)time);
    strprint(" ms\n");
    double chance = (double)well / (double)rounds;
    printchance(chance);
}

// функция для каждого потока для последовательной реализации
DWORD WINAPI Thread(LPVOID args) 
{
    ThreadArgs* structarg = (ThreadArgs*)args;  
    structarg->success = 0;
    structarg->rand = GetCurrentThreadId();//делаем число для рандома рандомным
    // обновление счетчика в случае успеха
    for (SIZE_T i = 0; i < structarg->numrounds; i++)
    {
        structarg->success += cards(&structarg->rand);
    }
    WaitForSingleObject(mutex, INFINITE);//МЬЮТЕКС чтобы общий счетчик не прибавлялся у всех сразу
    total += structarg->success;
    ReleaseMutex(mutex); // освобождение мьютекса для следующего потока
    return 0; 
}

LONGLONG micros()
{
    static LARGE_INTEGER freq = { 0 };
    LARGE_INTEGER counter;

    if (freq.QuadPart == 0)
        QueryPerformanceFrequency(&freq);

    QueryPerformanceCounter(&counter);
    return (counter.QuadPart * 1000000LL) / freq.QuadPart; // в микросекундах
}

//параллельный алгоритм!!! в нем много потоков
void parallel(SIZE_T rounds, SIZE_T numthreads)
{
    total = 0;

    HANDLE* threads = HeapAlloc(GetProcessHeap(), 0, numthreads * sizeof(HANDLE));
    ThreadArgs* structarg = HeapAlloc(GetProcessHeap(), 0, numthreads * sizeof(ThreadArgs));

    if (!threads || !structarg) {
        strprint("memory allocation error\n");
        return;
    }

    SIZE_T thread_round = rounds / numthreads;
    SIZE_T ost = rounds % numthreads;

    LONGLONG start = micros();

    for (SIZE_T i = 0; i < numthreads; i++) {
        structarg[i].thread_id = i;
        structarg[i].numrounds = thread_round + (i < ost ? 1 : 0);
        threads[i] = CreateThread(NULL, 0, Thread, &structarg[i], 0, NULL);

        if (threads[i] == NULL) {
            strprint("error creating thread\n");
            numprint(i);
            strprint("\n");
            numthreads = i; // ждём только то, что создали
            break;
        }
    }

    // 👇 БЕЗ ОГРАНИЧЕНИЯ 64 потоков
    for (SIZE_T i = 0; i < numthreads; i++) {
        WaitForSingleObject(threads[i], INFINITE);
        CloseHandle(threads[i]);
    }

    LONGLONG end = micros();
    LONGLONG duration = end - start;

    strprint("thread number ");
    numprint(numthreads);
    strprint(" - ");
    numprint(duration);
    strprint(" microseconds\n");

    double chance = (double)total / (double)rounds;
    printchance(chance);

    HeapFree(GetProcessHeap(), 0, threads);
    HeapFree(GetProcessHeap(), 0, structarg);
}



int main(int argc, char* argv[])
{
    SYSTEM_INFO sysinfo;
    GetSystemInfo(&sysinfo);
    size_t cernel = sysinfo.dwNumberOfProcessors;
    SIZE_T rounds = 1000000;  
    if (argc > 1) 
    {
        rounds = strtoint(argv[1]);
    }
    //запуск последовательного алгоритма
    one(rounds);
    
    mutex = CreateMutex(NULL, FALSE, NULL);
    if (mutex == NULL) 
    {
        strprint("cannot create mutex\n");
        return 1;
    }
    
    parallel(rounds, 1);
    parallel(rounds, 2);
    parallel(rounds, 3);
    strprint("the next thread is the number of cernels - ");
    numprint(cernel);
    strprint("\n");
    parallel(rounds, cernel);
    parallel(rounds, 32);
    parallel(rounds, 128);
    parallel(rounds, 256);

    CloseHandle(mutex);

    return 0;  
}