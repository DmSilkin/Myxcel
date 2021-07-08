# Myxcel
Упрощенная реализация **Excel**. Программа позволяет работать с ячейками на листе - писать туда числовые, текстовые значения и рассчитывать формулы, содержащие ссылки на другие ячейки. Также осуществляется поиск циклических зависимостей и корректность задания значения ячейке.  
В качестве лексического и синтаксического анализаторов используется **ANTLR**.

---
### Инструкция по запуску проекта на Windows
1. Установить **Java SE Runtime Environment 8**, а также **Java SE Development Kit 8**.  
2. Установить **ANTLR4**, следуя инструкциям с [официального сайта](https://www.antlr.org)
3. Скачать в папку с проектом файл **antlr-4.9.2-complete.jar**, создать папку **antlr4_runtime** и положить туда исходники с [официального репозитория](https://github.com/antlr/antlr4/tree/master/runtime/Cpp)  
4. Далее необходимо сгенерировать исполняемые файлы на **С++** для **ANTLR**. ` antlr -Dlanguage=Cpp Formula.g4  `
5. Собрать проект с помощью **CMake**.
---
### Использование Myxcel  
- Пример заполнения листа значениями:
```C++
        sheet->SetCell("A1"_pos, "300");
        sheet->SetCell("B1"_pos, "Hello");
        sheet->SetCell("A2"_pos, "3");
        sheet->SetCell("C2"_pos, "=A3 / A2");
        sheet->SetCell("C4"_pos, "=C2 + 8");
```
- Вывод значений на полученном листе на печать:
```C++
        sheet->PrintValues(std::cout);
```
|  | A  | B  | C  |
| :-----: | :-: | :-: | :-: |
| 1 | 300 | Hello |  |  
| 2 | 3 |  | 0 |  
| 3 | 0 |  |  |  
| 4 |  |  | 8 |

- Вывод текста на полученном листе на печать:
```C++
        sheet->PrintTexts(std::cout);
```
|  | A  | B  | C  |
| :-----: | :-: | :-: | :-: |
| 1 | 300 | Hello |  |  
| 2 | 3 |  | =A3/A2 |  
| 3 |  |  |  |  
| 4 |  |  | =C2+8 |

---
