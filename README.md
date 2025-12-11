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

компиляция 4
gcc -shared cos1.c -o cos1.dll -lm
gcc -shared sort1.c -o sort1.dll
gcc -shared cos2.c -o cos2.dll -lm
gcc -shared sort2.c -o sort2.dll
gcc program1.c cos1.c sort1.c -o program1.exe -lm -luser32
ИЛИ
gcc program1.c cos2.c sort2.c -o program1_v2.exe
gcc program2.c -o program2.exe -luser32
program1.exe
program2.exe 
