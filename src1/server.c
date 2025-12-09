#include <windows.h>
#include <string.h>

#define BUF_SIZE 1024

// Функция проверки на простоту
int is_prime(int n) 
{
    if (n <= 1) 
        return 0;
    if (n <= 3) 
        return 1;
    if (n % 2 == 0 || n % 3 == 0) 
        return 0;
    for (int i = 5; i * i <= n; i += 2) 
    {
        if (n % i == 0) 
            return 0;
    }
    return 1;
}

// Функция преобразования строки в число
int string_to_int(const char* str) 
{
    int result = 0;
    int sign = 1;
    int i = 0;
    if (str[0] == '-') 
    {
        sign = -1;
        i = 1;
    }
    while (str[i] >= '0' && str[i] <= '9') 
    {
        result = result * 10 + (str[i] - '0');
        i++;
    }
    return result * sign;
}

// Функция преобразования числа в строку
void int_to_string(int num, char* buffer) 
{
    if (num == 0) 
    {
        buffer[0] = '0';
        buffer[1] = '\0';
        return;
    }
    int i = 0;
    int is_negative = 0;
    if (num < 0) 
    {
        is_negative = 1;
        num = -num;
    }
    while (num > 0) //цифры получаются в обратном порядке
    {
        buffer[i++] = '0' + (num % 10);
        num /= 10;
    }
    if (is_negative) 
    {
        buffer[i++] = '-';
    }
    buffer[i] = '\0';
    // получаем цифры в нормальном порядке
    for (int j = i - 1, k = 0; j > k; j--, k++)
    {
        char temp = buffer[k];
        buffer[k] = buffer[j];
        buffer[j] = temp;
    }
}


int main()
{
    // Открываем файл для записи составных чисел
    //только запись, другие могут читать файл, создать или перезаписать, обычный файл
    HANDLE file = CreateFile("composite_numbers.txt", GENERIC_WRITE, FILE_SHARE_READ,
        NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE) 
    {
        const char error[] = "Error: cannot create file composite_numbers.txt\n";
        WriteFile(GetStdHandle(STD_ERROR_HANDLE), error, sizeof(error) - 1, NULL, NULL);
        return 1;
    }
    char buffer[BUF_SIZE];
    DWORD bytes_read, bytes_written;
    int continue_working = 1;
    while (continue_working) 
    {
        // читаем число от клиента(из pipe)
        if (!ReadFile(GetStdHandle(STD_INPUT_HANDLE), buffer, BUF_SIZE, &bytes_read, NULL)) 
        {
            const char error[] = "Error reading from client\n";
            WriteFile(GetStdHandle(STD_ERROR_HANDLE), error, sizeof(error) - 1, NULL, NULL);
            break;
        }
        if (bytes_read == 0) 
            break;
        // преобразуем в строку (убираем \r\n)
        buffer[bytes_read] = '\0';
        if (buffer[bytes_read - 2] == '\r') 
            buffer[bytes_read - 2] = '\0';
        else if (buffer[bytes_read - 1] == '\n')
            buffer[bytes_read - 1] = '\0';
        // преобразуем строку в число
        int number = string_to_int(buffer);
        // проверяем условия
        if (number < 0) 
        {
            // отрицательное число - завершаем работу
            const char response[] = "SHUTDOWN";//это отправляется родителю через pipe
            WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), response, sizeof(response) - 1, &bytes_written, NULL);
            continue_working = 0;
            const char log[] = "Negative number, shutting down\n";//причина выхода
            WriteFile(GetStdHandle(STD_ERROR_HANDLE), log, sizeof(log) - 1, NULL, NULL);
        }
        else if (number == 0 || number == 1) 
        {
            // Особые случаи
            const char response[] = "SHUTDOWN";  // Или другое сообщение
            WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), response, sizeof(response) - 1, &bytes_written, NULL);
            continue_working = 0;
        }
        else if (is_prime(number)) 
        {
            // простое число - завершаем работу
            const char response[] = "SHUTDOWN";
            WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), response, sizeof(response) - 1, &bytes_written, NULL);
            continue_working = 0;
            const char log[] = "prime number, shutting down\n";//причина выхода
            WriteFile(GetStdHandle(STD_ERROR_HANDLE), log, sizeof(log) - 1, NULL, NULL);
        }
        else 
        {
            // составное число - пишем в файл и продолжаем
            char number_str[32];
            int_to_string(number, number_str);
            // записываем в файл
            int len = 0;
            while (number_str[len]) 
                len++;
            WriteFile(file, number_str, len, &bytes_written, NULL);
            WriteFile(file, "\r\n", 2, &bytes_written, NULL);
            // отправляем ответ клиенту
            char response[BUF_SIZE];
            int_to_string(number, response);
            int response_len = 0;
            while (response[response_len]) 
                response_len++;
            WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), response, response_len, &bytes_written, NULL);
        }
    }
    CloseHandle(file);//системный вызов - закрытие файла
    return 0;
}
