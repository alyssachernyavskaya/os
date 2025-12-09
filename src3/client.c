#include <windows.h>
#include <string.h>

#define BUF_SIZE 1024
#define SHARED_MEM_SIZE 4096 //размер shared memory

//функция для проверки является ли введенная строчка числа
int is_number(const char* str) 
{
    if (*str == '-' || *str == '+') 
        str++;
    if (*str == '\0') 
        return 0;
    while (*str) 
    {
        if (*str < '0' || *str > '9') 
            return 0;
        str++;
    }
    return 1;
}

int main(int argc, char* argv[]) //имя разделенной памяти и имя семафоры
{
    if (argc != 3) //количество аргументов
    {
        const char error[] = "wrong count of arguments\n";
        WriteFile(GetStdHandle(STD_ERROR_HANDLE), error, sizeof(error) - 1, NULL, NULL);
        return 1;
    }

    const char* shared_mem_name = argv[1];
    const char* semaphore_name = argv[2];

    // открытие shared memory с полным доступом и без наследия дочернями процессами
    HANDLE hMapFile = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, shared_mem_name);
    if (hMapFile == NULL) 
    {
        const char error[] = "cannot open shared memory. start server first!\n";
        WriteFile(GetStdHandle(STD_ERROR_HANDLE), error, sizeof(error) - 1, NULL, NULL);
        return 1;
    }

    //отображение разделенной памяти в пространство процесса
    char* shared_mem = (char*)MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, SHARED_MEM_SIZE);
    if (shared_mem == NULL) 
    {
        const char error[] = "cannot map shared memory\n";
        WriteFile(GetStdHandle(STD_ERROR_HANDLE), error, sizeof(error) - 1, NULL, NULL);
        CloseHandle(hMapFile);
        return 1;
    }

    // открытие семафоры
    HANDLE hSemaphore = OpenSemaphore(SEMAPHORE_ALL_ACCESS, FALSE, semaphore_name);
    if (hSemaphore == NULL) 
    {
        const char error[] = "cannot open semaphore\n";
        WriteFile(GetStdHandle(STD_ERROR_HANDLE), error, sizeof(error) - 1, NULL, NULL);
        UnmapViewOfFile(shared_mem);
        CloseHandle(hMapFile);
        return 1;
    }

    const char ready[] = "client is ready. enter numbers\n";
    WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), ready, sizeof(ready) - 1, NULL, NULL);

    char buffer[BUF_SIZE];
    DWORD bytes_read;
    int running = 1;

    while (running) 
    {
        WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), "> ", 2, NULL, NULL);//вывод

        if (!ReadFile(GetStdHandle(STD_INPUT_HANDLE), buffer, BUF_SIZE, &bytes_read, NULL)) 
        {
            const char error[] = "error of input\n";
            WriteFile(GetStdHandle(STD_ERROR_HANDLE), error, sizeof(error) - 1, NULL, NULL);
            break;
        }

        // убираем \r\n
        if (bytes_read >= 2 && buffer[bytes_read - 2] == '\r' && buffer[bytes_read - 1] == '\n') 
        {
            buffer[bytes_read - 2] = '\0';
        }
        else if (bytes_read >= 1 && buffer[bytes_read - 1] == '\n') 
        {
            buffer[bytes_read - 1] = '\0';
        }

        // если введена пустая строчка
        if (buffer[0] == '\0') 
        {
            WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), "exit\n", 5, NULL, NULL);
            WaitForSingleObject(hSemaphore, INFINITE);//захват семафоы - то же что wait
            //теперь клиент может что-то делать, а сервер - нет
            strcpy(shared_mem, "EX");//записываем в память команду выхода и она отправится серверу
            ReleaseSemaphore(hSemaphore, 1, NULL);//отпускаем семафору, теперь и сервер может писать(счетчик+1)
            break;
        }

        
        if (!is_number(buffer)) 
        {
            const char error[] = "not valid number\n";
            WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), error, sizeof(error) - 1, NULL, NULL);
            continue;//продолжаем ввод
        }

        // отправка числа серверу
        WaitForSingleObject(hSemaphore, INFINITE);//захват семафоры,чтобы клиент мог писать в shared memory
        strcpy(shared_mem, buffer);//запись в shared memory введенного числа
        ReleaseSemaphore(hSemaphore, 1, NULL);//отпускаем симафору, счетчик +=1 и сервер моожет использовать

        //ожидание ответа от сервера
        int flag = 0;//флаг получен ли отет
        while (!flag && running)//пока ответ не получен
        {
            Sleep(100);//даем время получить ответ
            WaitForSingleObject(hSemaphore, INFINITE);//предполагается, что ответ получили
            //теперь захватываем симафору, чтоб обработать ответ
            if (strncmp(shared_mem, "STOP:", 5) == 0) 
            {
                // найдено простое число или отрицательное значит остановка сервера
                const char msg[] = "server stopped";
                WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), msg, sizeof(msg) - 1, NULL, NULL);
                WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), shared_mem + 5, strlen(shared_mem + 5), NULL, NULL);
                WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), "\n", 1, NULL, NULL);
                running = 0;//останавливаем цикл
                flag = 1;//ответ получен
            }
            else if (strcmp(shared_mem, "CONTINUE") == 0) 
            {
                // если получено составное число то продолжаем
                const char msg[] = "not prime number - continue\n";
                WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), msg, sizeof(msg) - 1, NULL, NULL);
                flag = 1;//ответ получен
            }
            else if (strcmp(shared_mem, "END") == 0) 
            {
                // если сервер завершил работу
                const char msg[] = "that's all\n";
                WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), msg, sizeof(msg) - 1, NULL, NULL);
                running = 0;//прерываем цикл обработки сообщения
                flag = 1;
            }

            if (flag)
            {
                shared_mem[0] = '\0';//добавляем в конец строчки символ конца строчки
            }
            ReleaseSemaphore(hSemaphore, 1, NULL);//отпускаем сервер, счетчик+=1
        }
    }
    CloseHandle(hSemaphore);
    UnmapViewOfFile(shared_mem);
    CloseHandle(hMapFile);
    return 0;
}