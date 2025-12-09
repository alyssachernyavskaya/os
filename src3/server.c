#include <windows.h>
#include <string.h>

#define BUF_SIZE 1024
#define SHARED_MEM_SIZE 4096

//функция проверки является ли число простым
int is_prime(int n) 
{
    if (n <= 1) return 0;//0 и 1 ни простые, ни составные
    if (n <= 3) return 1;// 2 и 3 т.е 1 простое число
    if (n % 2 == 0 || n % 3 == 0) return 0;
    for (int i = 5; i * i <= n; i += 2) 
    {
        if (n % i == 0) return 0;
    }
    return 1;
}

int main(int argc, char* argv[]) 
{
    if (argc != 3) //количество аргументов(имя программы и имена shared memory и семафоры
    {
        const char error[] = "wrong count of arguments\n";
        WriteFile(GetStdHandle(STD_ERROR_HANDLE), error, sizeof(error) - 1, NULL, NULL);
        return 1;
    }

    const char* shared_mem_name = argv[1];
    const char* semaphore_name = argv[2];

    // создание shared memory с использованием системного файла подкачки
    HANDLE hMapFile = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, SHARED_MEM_SIZE, shared_mem_name);
    if (hMapFile == NULL) 
    {
        const char error[] = "Error: cannot create shared memory\n";
        WriteFile(GetStdHandle(STD_ERROR_HANDLE), error, sizeof(error) - 1, NULL, NULL);
        return 1;
    }

    //отображение shared memory в пространство процессора
    char* shared_mem = (char*)MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, SHARED_MEM_SIZE);
    if (shared_mem == NULL) 
    {
        const char error[] = "cannot create shared memory\n";
        WriteFile(GetStdHandle(STD_ERROR_HANDLE), error, sizeof(error) - 1, NULL, NULL);
        CloseHandle(hMapFile);
        return 1;
    }

    // создание семафора с начальными значениями 1 по умолчанию и 1 как максимальное значение
    HANDLE hSemaphore = CreateSemaphore(NULL, 1, 1, semaphore_name);
    if (hSemaphore == NULL) 
    {
        const char error[] = "cannot create semaphore\n";
        WriteFile(GetStdHandle(STD_ERROR_HANDLE), error, sizeof(error) - 1, NULL, NULL);
        UnmapViewOfFile(shared_mem);
        CloseHandle(hMapFile);
        return 1;
    }

    // создание файла с результатами
    HANDLE file = CreateFile//системный вызов
    ("not prime numbers.txt",
        GENERIC_WRITE,
        0,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL);

    if (file == INVALID_HANDLE_VALUE) //если не получилось создать файл
    {
        const char error[] = "cannot create file\n";
        WriteFile(GetStdHandle(STD_ERROR_HANDLE), error, sizeof(error) - 1, NULL, NULL);
    }

    // инициализация shared memory
    shared_mem[0] = '\0';

    const char ready[] = "server is ready. enter numbers in client\n";
    WriteFile(GetStdHandle(STD_ERROR_HANDLE), ready, sizeof(ready) - 1, NULL, NULL);

    int running = 1;

    while (running) 
    {
        WaitForSingleObject(hSemaphore, INFINITE);//захват семафоры чтобы использовать shared memory
        // смотрим есть ли данные от клиента
        if (shared_mem[0] != '\0') //если получили данные от клиента
        {
            if (shared_mem[0] == 'E' && shared_mem[1] == 'X') //если получили команду выхода
            {
                const char msg[] = "client stopped\n";
                WriteFile(GetStdHandle(STD_ERROR_HANDLE), msg, sizeof(msg) - 1, NULL, NULL);
                running = 0;
                strcpy(shared_mem, "END");//пишем в shared память команду завершиться
            }
            else 
            {
                // обработка числа
                int num = atoi(shared_mem);//перевод числа из строки

                char str[100];
                strcpy(str, "processing number: ");
                strcat(str, shared_mem);
                WriteFile(GetStdHandle(STD_ERROR_HANDLE), str, strlen(str), NULL, NULL);
                WriteFile(GetStdHandle(STD_ERROR_HANDLE), "\n", 1, NULL, NULL);

                if (num < 0) 
                {
                    strcpy(shared_mem, "STOP:negative");
                    running = 0;
                }
                else if (num == 0 || num == 1) 
                {
                    strcpy(shared_mem, "STOP:01");
                    running = 0;
                }
                else if (is_prime(num)) 
                {
                    strcpy(shared_mem, "STOP:prime");
                    running = 0;
                }
                else 
                {
                    // если составное число то записываем в файл и продолжаем
                    if (file != INVALID_HANDLE_VALUE) 
                    {
                        DWORD bytes_written;
                        WriteFile(file, shared_mem, strlen(shared_mem), &bytes_written, NULL);
                        WriteFile(file, "\r\n", 2, &bytes_written, NULL);
                    }
                    strcpy(shared_mem, "CONTINUE");//отправляем команду продолжать
                }
            }
        }
        ReleaseSemaphore(hSemaphore, 1, NULL);//отпускаем семафору ура
        Sleep(100);//ждем чтоб полегче стало
    }
    const char finish[] = "server finished\n";
    WriteFile(GetStdHandle(STD_ERROR_HANDLE), finish, sizeof(finish) - 1, NULL, NULL);

    if (file != INVALID_HANDLE_VALUE) 
    {
        CloseHandle(file);
    }
    CloseHandle(hSemaphore);
    UnmapViewOfFile(shared_mem);
    CloseHandle(hMapFile);
    return 0;
}