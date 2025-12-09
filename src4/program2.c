//динамическая загрузка: библиотеки загружаются прямо во время выполнения и можно менять реализацию

#include <windows.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// типы указатели на функции, т.к. компилятор не знает о них заранее
typedef float (*cosdx_func)(float, float);
typedef int* (*sort_func)(int*, size_t);

// глобальные переменные для загружаемых библиотек
HMODULE cos_lib = NULL;// дескриптор библиотеки производной
HMODULE sort_lib = NULL;//дескриптор библиотеки сортировки
cosdx_func cosdx_ptr = NULL;// указатель на функцию cosdx
sort_func sort_ptr = NULL;// указатель на функцию sort

int current_version = 1;// данная реализация библиотеки (линковка или днамическая)

// функции ввода/вывода
void win_write(const char* str) 
{
    HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE);//дескриптор вывода
    DWORD bytesWritten;//количество записанных байт
    WriteFile(hStdout, str, (DWORD)strlen(str), &bytesWritten, NULL);//вывод
}

void win_write_int(int value)//вывод числа
{
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "%d", value);
    win_write(buffer);
}

void win_write_float(float value) //вывод float для косинуса
{
    char buffer[64];
    snprintf(buffer, sizeof(buffer), "%.6f", value);
    win_write(buffer);
}

void win_write_array(int* arr, size_t n) //вывод массива для сортировки
{
    char buffer[64];
    for (size_t i = 0; i < n; i++) 
    {
        snprintf(buffer, sizeof(buffer), "%d ", arr[i]);
        win_write(buffer);
    }
}

int win_read_line(char* buffer, size_t size) //считывание
{
    HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
    DWORD bytesRead;

    if (!ReadFile(hStdin, buffer, (DWORD)size - 1, &bytesRead, NULL)) 
    {
        return 0;
    }

    buffer[bytesRead] = '\0';

    // убираем /n /r
    char* cr = strchr(buffer, '\r');
    if (cr) *cr = '\0';
    char* lf = strchr(buffer, '\n');
    if (lf) *lf = '\0';

    return 1;
}

// загружаем библиотеку
void load_libraries(int version) 
{
    char cos_lib_name[64];
    char sort_lib_name[64];

    if (version == 1) //если введено 1 то первая реализация
    {
        strcpy(cos_lib_name, "cos1.dll");
        strcpy(sort_lib_name, "sort1.dll");
    }
    else 
    {
        strcpy(cos_lib_name, "cos2.dll");
        strcpy(sort_lib_name, "sort2.dll");
    }

    // чистим старые библиотеки после смены реализации
    if (cos_lib) 
        FreeLibrary(cos_lib);
    if (sort_lib) 
        FreeLibrary(sort_lib);

    // Загружаем новые
    cos_lib = LoadLibraryA(cos_lib_name); //загружает DLL в память
    if (!cos_lib) 
    {
        char error[128];
        snprintf(error, sizeof(error), "error dll %s\n", cos_lib_name);
        win_write(error);
        return;
    }

    sort_lib = LoadLibraryA(sort_lib_name);
    if (!sort_lib) 
    {
        char error[128];
        snprintf(error, sizeof(error), "error dll %s\n", sort_lib_name);
        win_write(error);
        FreeLibrary(cos_lib);//выход из программы поэтому 1ую чистим тоже
        cos_lib = NULL;
        return;
    }

    // получаем указатели на функции через сисколлы
    cosdx_ptr = (cosdx_func)GetProcAddress(cos_lib, "cosdx");//находит адрес функции по её имени
    sort_ptr = (sort_func)GetProcAddress(sort_lib, "sort");

    if (!cosdx_ptr || !sort_ptr) 
    {
        win_write("cant find functions in libraries\n");
        FreeLibrary(cos_lib);
        FreeLibrary(sort_lib);
        cos_lib = NULL;
        sort_lib = NULL;
        return;
    }

    char msg[128];
    snprintf(msg, sizeof(msg), "got to version %d\n", version);
    win_write(msg);
}

int main() 
{
    char input[256];

    win_write(" program 2: dynamic\n");
    win_write("commands:\n");
    win_write("  0 - choose version (1 or 2)\n");
    win_write("  1 a dx - derivative of cos(x)\n");
    win_write("  2 n a1 a2 ... an - sort array\n");
    win_write("  exit - exit program\n");

    // загрузка библиотек
    load_libraries(current_version);

    while (1) 
    {
        win_write("\n> ");

        if (!win_read_line(input, sizeof(input))) 
        {
            break;
        }

        // если введен выход
        if (strcmp(input, "exit") == 0) 
        {
            win_write("exiting\n");
            break;
        }

        if (strlen(input) == 0) 
        {
            continue;
        }

        // если введен 0 переключаем версию
        if (input[0] == '0') 
        {
            current_version = (current_version == 1) ? 2 : 1;
            load_libraries(current_version);
        }
        // введена 1 - ищем производную
        else if (input[0] == '1') 
        {
            if (!cosdx_ptr) 
            {
                win_write("error loading\n");
                continue;
            }

            float a = 0, dx = 0;
            int count = sscanf(input + 1, "%f %f", &a, &dx);

            if (count == 2) 
            {
                float result = cosdx_ptr(a, dx);//вызов функции через указатель
                char msg[128];
                snprintf(msg, sizeof(msg), "cos'(%f) with dx=%f = ", a, dx);
                win_write(msg);
                win_write_float(result);
                win_write("\n");

                // показываем какая версия использовалась
                snprintf(msg, sizeof(msg), " (using version %d: ", current_version);
                win_write(msg);
                win_write(current_version == 1 ? "realization 1)\n" : "realization 2)\n");
            }
            else 
            {
                win_write("you should print 2 numbers\n");
            }
        }
        // ведено 2 это сортировка
        else if (input[0] == '2') 
        {
            if (!sort_ptr) 
            {
                win_write("loading error\n");
                continue;
            }

            char* token = strtok(input, " ");
            token = strtok(NULL, " "); // пропускаем 2

            if (!token) 
            {
                win_write("print the array size\n");
                continue;
            }

            int n = atoi(token);//преобразуем элемент массива в число
            if (n <= 0 || n > 100) 
            {
                win_write("please print the array size 1-100\n");
                continue;
            }

            int* array = (int*)malloc(n * sizeof(int));
            if (!array) 
            {
                win_write("memory error\n");
                continue;
            }

            // считываниеи элементов массива
            int valid = 1;
            for (int i = 0; i < n; i++) 
            {
                token = strtok(NULL, " ");
                if (!token)//значит введено мало чисел
                {
                    win_write("not enough array elements\n");
                    valid = 0;
                    break;
                }
                array[i] = atoi(token);
            }

            if (valid) 
            {
                win_write("printed array: ");
                win_write_array(array, n);
                win_write("\n");

                int* sorted = sort_ptr(array, n);//указатель на функцию сортировки
                if (sorted) 
                {
                    win_write("sorted array:   ");
                    win_write_array(sorted, n);

                    char msg[64];
                    snprintf(msg, sizeof(msg), " (using version %d: ", current_version);
                    win_write(msg);
                    win_write(current_version == 1 ? "bubble sort)\n" : "quick sort)\n");

                    free(sorted);
                }
                else 
                {
                    win_write("sorting error\n");
                }
            }

            free(array);
        }
        else 
        {
            win_write("print 0, 1, or 2.\n");
        }
    }

    // освобождение библиотек перед выходом
    if (cos_lib) FreeLibrary(cos_lib);
    if (sort_lib) FreeLibrary(sort_lib);

    return 0;
}