//статическая линковка: подключение библиотеки на этапе компиляции, т.е. программа знает о функциях заранее
//и ос сразу загружает библиотеки

#include "contract.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <windows.h>

// Функции ввода/вывода
void win_write(const char* str) 
{
    HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE);//дескриптор вывода
    DWORD bytesWritten;//количество записанных байт
    WriteFile(hStdout, str, (DWORD)strlen(str), &bytesWritten, NULL);//вывод
}

void win_write_float(float value) //вывод числа float для косинуса
{
    char buffer[64];
    snprintf(buffer, sizeof(buffer), "%.6f\n", value);
    win_write(buffer);
}

void win_write_array(int* arr, size_t n) //вывод массива
{
    char buffer[64];
    for (size_t i = 0; i < n; i++) 
    {
        snprintf(buffer, sizeof(buffer), "%d ", arr[i]);
        win_write(buffer);
    }
    win_write("\n");
}

int win_read_line(char* buffer, size_t size)//ввод
{
    HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
    DWORD bytesRead;

    if (!ReadFile(hStdin, buffer, (DWORD)size - 1, &bytesRead, NULL)) 
    {
        return 0;
    }

    buffer[bytesRead] = '\0';

    // Убираем \r и \n
    char* cr = strchr(buffer, '\r');
    if (cr) 
        *cr = '\0';
    char* lf = strchr(buffer, '\n');
    if (lf) 
        *lf = '\0';

    return 1;
}

int main() 
{
    char input[256];//буфер для ввода

    win_write(" program 1: static linking \n");
    win_write(" commands:\n");
    win_write("  0 - show library info\n");
    win_write("  1 a dx - derivative of cos(x) at point 'a' with step 'dx'\n");
    win_write("  2 n a1 a2 ... an - sort array of n numbers\n");
    win_write("  exit - exit program\n");

    while (1) 
    {
        win_write("\n> ");

        if (!win_read_line(input, sizeof(input))) 
        {
            break;
        }

        // если выход
        if (strcmp(input, "exit") == 0) 
        {
            win_write("exiting\n");
            break;
        }

        if (strlen(input) == 0) //может быть введен только 1 символ
        {
            continue;
        }

        // если введен 0: вывод информации
        if (input[0] == '0') 
        {
            win_write(" program is statically linked\n");
            win_write("  - cos1.dll (the 1st realization)\n");
            win_write("  - sort1.dll (bubble sort)\n");
            win_write("you can't switch realizations\n");
        }

        // Команда 1: производная
        else if (input[0] == '1') 
        {
            float a = 0, dx = 0;
            int count = sscanf(input + 1, "%f %f", &a, &dx);

            if (count == 2) //должно быть введено 2 числа
            {
                float result = cosdx(a, dx);
                char msg[128];
                snprintf(msg, sizeof(msg), "cos'(%f) with dx=%f = ", a, dx);//запись в массив
                win_write(msg);//вывод в массив
                win_write_float(result);//вывод результата
            }
            else 
            {
                //значит было введено не 2 числа
                win_write("you should print 2 numbers\n");
            }
        }
        // введено 2: сортировка
        else if (input[0] == '2') 
        {
            char* token = strtok(input, " ");
            token = strtok(NULL, " "); // Пропускаем "2" - на вход подаются 2 и размер массива

            if (!token) 
            {
                win_write("error: print the array size\n");
                continue;
            }

            int n = atoi(token);//преобразовываем элемент массива в число
            if (n <= 0 || n > 100) 
            {
                win_write("error: please print array size between 1-100\n");
                continue;
            }
            //выделяем память под динамический массив
            int* array = (int*)malloc(n * sizeof(int));
            if (!array) 
            {
                win_write("memory error\n");
                continue;
            }

            // считывание элементов массива
            int valid = 1;
            for (int i = 0; i < n; i++) 
            {
                token = strtok(NULL, " ");//так получаем указатель на следующее число
                if (!token) //ввели слишком мало элементов массива
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

                int* sorted = sort(array, n);
                if (sorted) 
                {
                    win_write("sorted array:   ");
                    win_write_array(sorted, n);
                    free(sorted);
                }
                else 
                {
                    win_write("error in sorting\n");
                }
            }

            free(array);
        }
        else {
            win_write("print 0, 1, or 2.\n");
        }
    }

    return 0;
}