//сортировка кучей!!!!

#include <stdlib.h>
#include <string.h>

static void quick_sort(int* arr, int low, int high) //2 и 3 аргументы это индексы
{
    if (low < high) 
    {
        int central = arr[(low + high) / 2];//серединка
        int i = low - 1;//индекс слева
        int j = high + 1;//индекс справа

        while (1) 
        {
            do { i++; }
            while (arr[i] < central);//поиск элемента больше серединки слева

            do { j--; } 
            while (arr[j] > central);//поиск элемента меньше серединки справа

            if (i >= j) 
                break;//если индексы пересеклись

            int temp = arr[i];//меняем местами
            arr[i] = arr[j];
            arr[j] = temp;
        }
        //рекурсия: сортирум левую и правую части)
        quick_sort(arr, low, j);
        quick_sort(arr, j + 1, high);
    }
}

int* sort(int* array, size_t n) 
{
    if (n == 0) return NULL;

    // копируем массив в динамический как в пузырьковой сортировке
    int* result = (int*)malloc(n * sizeof(int));
    if (!result) 
        return NULL;
    memcpy(result, array, n * sizeof(int));

    // выполняем сортировку
    quick_sort(result, 0, n - 1);

    return result;
}