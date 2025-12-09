КОМПИЛЯЦИЯ 1
cd C:\Users\Алиска\Documents\mai\osi\lr1
gcc client.c -o client.exe -lkernel32 -luser32
gcc server.c -o server.exe -lkernel32 -luser32
client.exe
после тестирования
type composite_numbers.txt

компиляция 2
cd C:\Users\Алиска\Documents\mai\osi\1122
gcc -o program.exe Source.c
program.exe 1000000

сборка третьей
в 1 командной строке:
cd C:\Users\Алиска\Documents\mai\osi\lr333
gcc -o server.exe server.c gcc -o client.exe client.c
в другой
cd C:\Users\Алиска\Documents\mai\osi\lr333
server.exe MyMemory MySemaphore
и снова в первой
client.exe MyMemory MySemaphore
