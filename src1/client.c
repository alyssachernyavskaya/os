#include <windows.h>
#include <string.h>

#define BUF_SIZE 1024

// ‘ункци€ проверки, что строка - целое число
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

int main() 
{
    
    SECURITY_ATTRIBUTES sa; //переменна€ типа ... (кто может наследовать и получить доступ к pipes)
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;  // разрешить наследование pipes
    sa.lpSecurityDescriptor = NULL;//использовать настройки безопасности по умолчанию

    // —оздаем каналы pipes 
    HANDLE to_child_read, to_child_write;   // родитель - ребенок
    HANDLE from_child_read, from_child_write; // ребенок - родитель

    // первый pipe (родитель пишет, ребенок читает)
    if (!CreatePipe(&to_child_read, &to_child_write, &sa, 0)) 
    {
        const char error[] = "Error: cannot create pipe to child\n";
        //получаем дескриптор об ошибке
        WriteFile(GetStdHandle(STD_ERROR_HANDLE), error, sizeof(error) - 1, NULL, NULL);
        return 1;
    }

    // —оздаем второй канал (ребенок пишет, родитель читает)
    if (!CreatePipe(&from_child_read, &from_child_write, &sa, 0)) 
    {
        const char error[] = "Error: cannot create pipe from child\n";
        WriteFile(GetStdHandle(STD_ERROR_HANDLE), error, sizeof(error) - 1, NULL, NULL);
        return 1;
    }

    // Ќастраиваем информацию дл€ запуска
    STARTUPINFO start_info;//как запустить процесс
    PROCESS_INFORMATION proc_info;//информаци€ о созданном процессе

    ZeroMemory(&start_info, sizeof(start_info));//обнуление структуры создани€ процесса
    start_info.cb = sizeof(start_info);//размер структуры создани€ процесса
    start_info.dwFlags = STARTF_USESTDHANDLES;//использование handles дл€ потоков ввода/вывода
    //то, что написано в строке сверху:
    start_info.hStdInput = to_child_read;// ребенок читает отсюда
    start_info.hStdOutput = from_child_write;// ребенок пишет сюда
    start_info.hStdError = GetStdHandle(STD_ERROR_HANDLE);

    ZeroMemory(&proc_info, sizeof(proc_info));//обнуление структуры информации о процессе
    // запуск сервера
    if (!CreateProcess("server.exe", NULL, NULL, NULL, TRUE, 0, NULL, NULL, &start_info, &proc_info)) 
    {
        const char error[] = "Error: cannot start server.exe\n";
        WriteFile(GetStdHandle(STD_ERROR_HANDLE), error, sizeof(error) - 1, NULL, NULL);
        return 1;
    }

    // «акрываем ненужные концы pipes в родителе
    CloseHandle(to_child_read); 
    CloseHandle(from_child_write);

    const char str[] = "Enter numbers (empty line to exit):\n";
    WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), str, sizeof(str) - 1, NULL, NULL);
    char buffer[BUF_SIZE];
    DWORD bytes_read, bytes_written;//хран€т количество прочитанных и записанных байтов
    int should_exit = 0;
    while (!should_exit)
    {
        //считывание ввода пользовател€
        if (!ReadFile(GetStdHandle(STD_INPUT_HANDLE), buffer, BUF_SIZE, &bytes_read, NULL))
        {
            const char error[] = "Error reading input\n";
            WriteFile(GetStdHandle(STD_ERROR_HANDLE), error, sizeof(error) - 1, NULL, NULL);
            break;
        }
        // провер€ем пустую строку дл€ выхода
        if (bytes_read == 2 && buffer[0] == '\r' && buffer[1] == '\n')
        {
            const char exiting[] = "Exiting...\n";
            WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), exiting, sizeof(exiting) - 1, NULL, NULL);
            break;
        }
        // проверка что введено число
        buffer[bytes_read] = '\0';
        if (buffer[bytes_read - 2] == '\r') 
            buffer[bytes_read - 2] = '\0';
        else if (buffer[bytes_read - 1] == '\n') //убираем символы \r\n из конца строки
            buffer[bytes_read - 1] = '\0';
        if (!is_number(buffer))
        {
            const char notnumber[] = "Error: please enter a number\n";
            WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), notnumber, sizeof(notnumber) - 1, NULL, NULL);
            continue;//надеемс€,что хоть со 2го раза введут число и продолжаем
        }
        // отправка числа серверу(запись числа в pipe к ребенку)
        if (!WriteFile(to_child_write, buffer, bytes_read, &bytes_written, NULL))
        {
            const char error[] = "Error sending to server\n";
            WriteFile(GetStdHandle(STD_ERROR_HANDLE), error, sizeof(error) - 1, NULL, NULL);
            break;
        }
        // ожидание ответа от сервера(чтение ответа из pipe от ребенка)
        if (!ReadFile(from_child_read, buffer, BUF_SIZE, &bytes_read, NULL))
        {
            const char error[] = "Error reading from server\n";
            WriteFile(GetStdHandle(STD_ERROR_HANDLE), error, sizeof(error) - 1, NULL, NULL);
            break;
        }
        // проверка не нужно ли завершитьс€
        if (bytes_read >= 8 && strncmp(buffer, "SHUTDOWN", 8) == 0)
        {
            const char shutdown[] = "Server requested shutdown (prime or negative number)\n";
            WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), shutdown, sizeof(shutdown) - 1, NULL, NULL);
            should_exit = 1;//если ребенок прислал shutdown - устанавливаем флаг выхода
        }
        else
        {
            // ¬ыводим результат(он хранитс€ в буфере)
            WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), buffer, bytes_read, &bytes_written, NULL);
            const char newline[] = "\n";
            WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), newline, sizeof(newline) - 1, NULL, NULL);
        }
    }
    // «акрываем handles и ждем завершени€ ребенка
    CloseHandle(to_child_write);
    CloseHandle(from_child_read);
    CloseHandle(proc_info.hProcess);//последнее слово- им€ дл€ управлени€ процессом
    CloseHandle(proc_info.hThread);//последнее слово - им€ дл€ управлени€ потоками внутри процесса

    return 0;
}