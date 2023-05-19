# Нативный компилятор ReverseLang под архитектуру x64

##  Описание

Компилятор — программа, переводящая написанный на языке программирования текст в бинарный исполняемый файл.
В конкретном случае — с ReverseLang в ELF файл для архитектуры amd64. С синтаксисом ReverseLang можно ознакомится в 
[разделе с краткой справкой](#синтаксис-reverselang)

## Использование
### Сборка компилятора

Для сборки компилятора выполните команды:
```bash
$ git clone https://github.com/foxidokun/x64_compiler # Склонируйте репозиторий
$ cd x64_compiler && make all                         # Собрать все неопходимые бинарники
```

После этого компилировать ReverseLang можно с помощью скрипта `compile.sh`, передав ему два аргумента — файл с исходным кодом на языке Reverselang и путь, по которому сохранять собранный исполняемый файл.

Например, чтобы скомпилировать и запустить программу для решения квадратного уравнения, доступную как пример в `examples/`:
```bash 
    $ ./compile.sh examples/quad.edoc /tmp/quad.bin
    $ /tmp/quad.bin  
        INPUT: 1     # x^2-4x+3 = 0
        INPUT: -4
        INPUT: 3
        OUTPUT: 2.00 # 2 roots
        OUTPUT: 1.00 # x = 1
        OUTPUT: 3.00 # x = 3
```

### Синтаксис ReverseLang

1. Все переменные имеют один тип -- знаковые 64-битные числа с фиксированной точностью в два десятичных знака.
2. Все функции обязательно имеют возвращаемое значение.
3. Стандартная библиотека языка содержит 5 функций: `input / output / sqrt / sin / cos`.
4. С переменными можно проводить следующие математические операции: `+, -, /, *`.
5. Доступны логические операции сравнения: `>, >=, <, <=`, а также логические И и ИЛИ (`&& и ||`) и отрицание (`!`).
6. В языке есть поддержка функций, циклов while и if-else блоков.

<details>
  <summary>Грамматика</summary>

```
Program        ::= PROG_BEG (SubProgram | Func)* PROG_END
Func           ::= L_BRACKET (NAME (SEP NAME)) R_BRACKET NAME FN FUNC_OPEN_BLOCK Subprogram FUNC_CLOSE_BLOCK
SubProgram     ::= (FlowBlock)+
FlowBlock      ::= IfBlock | WhileBlock | OPEN_BLOCK Body CLOSE_BLOCK | Body
WhileBlock     ::= L_BRACKET Expression R_BRACKET WHILE OPEN_BLOCK Body CLOSE_BLOCK
IfBlock        ::= L_BRACKET Expression R_BRACKET IF OPEN_BLOCK Body CLOSE_BLOCK (ELSE OPEN_BLOCK Body CLOSE_BLOCK)
Body           ::= (Line)+
Line           ::= BREAK Expression RETURN | BREAK Expression (= NAME (LET))
Expression     ::= OrOperand (|| OrOperand)+
OrOperand      ::= AndOperand (&& AndOperand)+
AndOperand     ::= CompOperand (<=> CompOperand)
CompOperand    ::= AddOperand  ([+-] AddOperand)*
AddOperand     ::= MulOperand  ([/ *] MulOperand )*
MulOperand     ::= GeneralOperand (NOT)
GeneralOperand ::= Quant | L_BRACKET Expression R_BRACKET
Quant          ::= VAR | VAL | INPUT | BuiltInFunc | L_BRACKET (Expression (SEM Expression)) R_BRACKET NAME
BuiltInFunc    ::= L_BRACKET Expression R_BRACKET (PRINT|SQRT|SIN)
```
</details>


Синтаксис ReverseLang является C-подобным с небольшим отличием: каждую строку стоит читать справа налево. Так, например,
следующий код на C
```c
int func (int a, int b, int c) {
    if (a > b) {
        return c;
    } else {
        return a / b;
    }   
}
```
на ReverseLang примет следующий вид
```rust
(c, b, a) func fn
[
    (b > a) if
    {
        ; c return
    } else {
        ; b / a return
    }
]
```

Больше примеров можно найти в `examples/`

## Принцип работы

### Архитектура компилятора

Компилятор разбит на три ключевых компонента, поэтапно обрабатывающих исходный код и использующих в качестве внутреннего представления абстрактное 
синтаксическое дерево (_abstract syntax tree_, AST). В синтаксическом дереве внутренние вершины сопоставлены с операторами языка программирования, а листья — с соответствующими операндами.

[//]: # (TODO Может выкинуть пояснение, что такое AST дерево? Все же шарят...) 

**Фронтенд**
1. Лексический анализ разбивает исходный код на логические кванты (лексемы) — числа, ключевые слова, имена переменных и функций.
2. Синтаксический анализ собирает из лексем синтаксические конструкции, такие как функции и циклы, используя алгоритм рекурсивного спуска ([описание алгоритма](https://en.wikipedia.org/wiki/Recursive_descent_parser)). 
3. В процессе рекурсивного спуска строится абстрактное синтаксическое дерево ()`*`, которое является итоговым результатом фронтенда.

**Промежуточный оптимизатор (midleend)**

Оптимизатор принимает на вход AST и пытается упростить его, не нарушая при этом логику работы программы. В данный момент из оптимизаций применяется только вычисление 
константных выражений: обнаружив конструкцию из математических или логических операций над числами, оптимизатор вычисляет ее и заменяет на результат вычисления. 

**Бэкенд** 

Бэкенд принимает на вход оптимизированное AST дерево и обходит его в postorder порядке, генерируя машинный код для каждой вершины.
Таким образом при исполнении кода любой операции код вычисления ее операндов уже будет выполнен.

[//]: # (Вставить graphviz картинку AST дерева)

#### Обоснование архитектуры компилятора
Такая архитектура позволяет переиспользовать общие куски при написании компиляторов других языков или под 
другие архитектуры. Так, например, компиляторы одного языка под `x64` и `arm` могут использовать общие
фронтенд и оптимизатор, а компиляторы двух разных языков под одну архитектуру могут переиспользовать оптимизатор и бэкенд.

Например, поскольку фронтенды языков [ICPC](https://github.com/diht404/language) и [kaban54's lang](https://github.com/kaban54/language)
имеют совместимый формат AST, эти языки возможно скомпилировать с использованием бекенда из этого репозитория.

Подобная архитектура применяется в семействе компиляторов `GCC`, а также в основанных на `llvm` компиляторах, правда в последних в качестве 
внутреннего представления используется не AST, а линейное представление (LLVM IR).

### Устройство бекенда

[//]: # (Вместо IR = «промежкточное представление бэкенда &#40;Backend IR&#41;»)

[//]: # (Но при этом в устройстве компилятора термин IR не использовать)

[//]: # (Упомянуть двухпроходную схему)

Сам бекенд также имеет модульную структуру. Этап компиляции AST в машинный код разбит на три этапа: 
1. AST компилируется в линейное промежуточное представление (Backend IR) — массив структур, являющихся ассемблерным кодом
для абстрактного стекового процессора (подробнее далее в секции IR).
2. После получения IR можно начать выполнять оптимизационные проходы — убирать последовательные `push / pop`, например (_backend optimisations_). Однако в данный момент никакие оптимизации в бэкенде не применяются.
3. Далее этот IR транслируется в инструкции для конкретной архитектуры процессора.

#### Обоснование архитектуры бекенда
Наличие Backend IR, как и в случае с AST в компиляторе в целом, позволяет переиспользовать куски кода. Наличие IR
позволяет объединить трансляцию AST в набор элементарных действий для процессорных архитектур схожего типа или для 
различных выходных форматов. Благодаря IR данный компилятор помимо компиляции в бинарный файл умеет работать в JIT режиме
с минимальным дублированием кода.

[//]: # (Про форматы хуйня какая-то, переформулировать)

#### IR

В данном компиляторе в качестве IR используется связный список структур
```c++
    struct instruction_t {
        instruction_type_t type;            // Тип инструкции — push / add / call / etc
        struct {                            // Какие аргументы требуются инструкции
            unsigned char need_imm_arg: 1;  // - Константа
            unsigned char need_reg_arg: 1;  // - Регистр
            unsigned char need_mem_arg: 1;  // - Извлекается ли аргумент из памяти
        };

        unsigned char reg_num;              // Номер регистра-аргумента (если используется)
        uint64_t imm_arg;                   // Константный аргумент (если используется)

        size_t index;                       // Номер инструкции в IR
        instruction_t *next;                // Указатель на следующую структуру
    };
```

При этом IR рассчитан на абстрактный стековый процессор, а потому имеет следующие инструкции:

| Команда            | Действие                                                                                                                                               |
|--------------------|--------------------------------------------------------------------------------------------------------------------------------------------------------|
| `push imm/reg/mem` | Положить в стек константу, значение из регистра или из оперативной памяти по адресу `reg + imm`, где один из операндов, `reg` или `imm`, опционален.   |
| `pop reg/mem`      | Достать из стека значение и положить в регистр или оперативную память                                                                                  |
| `add/sub/mul/div`  | Арифметические операции с двумя верхними элементами на стеке (верхний элемент стека является правым операндом)                                         |
| `sqrt / sin / cos` | Арифметические операции с один элементом на стеке                                                                                                      |
| `call/jmp/j?? imm` | Совершить переход на адрес (номер структуры в IR), записанный в константном аргументе. `j??` обозначает условные переходы (`ja, jae, jb, jbe, je, jne`) |
| `inp / out`        | Ввод / вывод верхнего числа со стека                                                                                                                   |
| `ret`              | Команда возврата из функции, обратная к call                                                                                                           |
| `halt`             | Остановка программы, аналогичная функции `abort()` в C                                                                                                 |

При этом все инструкции поглощают свои операнды, если таковые имеются, и при наличии возвращаемого значения, кладут его на верхушку стека. 

### Структура ELF файла
![img.png](images/elf_structure.png)

В результате компиляции создается исполняемый ELF файл, содержащий оттранслированный код и код стандартной библиотеки.

Для этого в исполняемый файл записываются следующие заголовки:

1. **Заголовок ELF файла** содержит общую информацию про бинарник: архитектуру, адрес входа, количество и местоположение
заголовков сегментов. 

```c++
const Elf64_Ehdr ELF_HEADER = {
    .e_ident = {ELFMAG0, ELFMAG1, ELFMAG2, ELFMAG3, // Magic signature
                ELFCLASS64,                         // 64-bit system
                ELFDATA2LSB,                        // LittleEndian / BigEndian
                EV_CURRENT,                         // Version = Current
                ELFOSABI_NONE,                      // Non specified system
                0
                },
                
    .e_type    = ET_EXEC,                      // File type = Executable
    .e_machine = EM_X86_64,                    // Arch = amd64
    .e_version = EV_CURRENT,                   // Version = Current
    
    .e_entry   = 0x401000,                     // Fixed load addr
    
    .e_phoff    = sizeof(Elf64_Ehdr),          // Offset of program header table. We took size of elf header
    .e_shoff    = 0,                           // Offset of segment header table. Not used => 0
    
    .e_flags    = 0,                           // Extra flags: no flags
    .e_ehsize   = sizeof(Elf64_Ehdr),	       // Size of this header.
    
    .e_phentsize = sizeof(Elf64_Phdr),         // Size of Program header table entry.
    .e_phnum     = NUM_PHEADERS,               // Number of pheader entries. (system + stdlib + code + ram)
    
    .e_shentsize = sizeof(Elf64_Shdr),         // Size of Segment header entry.
    .e_shnum     = 0,                          // Number of segments in programm.
    .e_shstrndx  = 0,                          // Index of string table. (Explained in further parts).
};

```

2. **Заголовки сегментов** содержат информацию про каждый сегмент программы, его права, адрес загрузки и размер.
Данный компилятор использует только 4 сегмента:

* **Служебный сегмент**
```c++
Elf64_Phdr SYSTEM_PHEADER = {
        .p_type   = PT_LOAD    , /* [PT_LOAD] */
        .p_flags  = PF_R       , /* read */
        .p_offset = 0          , /* (bytes into file) */
        .p_vaddr  = 0x400000   , /* (virtual addr at runtime) */
        .p_paddr  = 0x400000   , /* (physical addr at runtime) */
        .p_filesz = sizeof(Elf64_Ehdr) + NUM_PHEADERS * sizeof(Elf64_Phdr), /* (bytes in file) */
        .p_memsz  = sizeof(Elf64_Ehdr) + NUM_PHEADERS * sizeof(Elf64_Phdr), /* (bytes in mem at runtime) */
        .p_align  = 4096       , /* (min mem alignment in bytes) */
};
```

Данный заголовок говорит загрузчику ELF файла скопировать ELF заголовки по адресу `0x400000` с правами только на чтение, он присутствует во всех исполняемых ELF файлах.

* **Сегмент стандартной библиотеки**
```c++
Elf64_Phdr STDLIB_PHEADER = {
        .p_type   = PT_LOAD,
        .p_flags  = PF_R | PF_X,           /* Read & Execute */
        .p_offset = 4096,                  /* (bytes into file) */
        .p_vaddr  = 0x403000,              /* (virtual addr at runtime) */
        .p_paddr  = 0x403000,              /* (physical addr at runtime) */
        .p_filesz = x64::STDLIB_SIZE,      /* (bytes in file) */
        .p_memsz  = x64::STDLIB_SIZE,      /* (bytes in mem at runtime) */
        .p_align  = 4096,                  /* (min mem alignment in bytes) */
};
```

Согласно этому заголовку код стандартной библиотеки будет загружен по адресу `0x401000` с правами на чтение и исполнение.
Поскольку код из исполняемого файла на самом деле не копируется, а отображается в память, то все сегменты обязаны
начинаться с адресов, кратных размеру страницы — 4096 байт. Поэтому стандартная библиотека записывается в бинарник начиная с 
4096 байта, а не сразу после заголовков.

* **Сегмент сгенерированного кода**

Сегмент сгенерированного кода отличается от стандартной библиотеки лишь адресом загрузки — `0x401000`, совпадающим 
с адресом входа, прописанным в ELF заголовке (поле `.e_entry`). Таким образом после загрузки ELF файла в память начинает
исполняться именно этот сегмент.

* **Сегмент оперативной памяти**

Главное отличие сегмента оперативной памяти от остальных — он не занимает места на диске. Вместо этого при загрузке ELF файла
операционная система сама создаст сегмент заданного размера и заполнит его нулями. Выражается это в нулевом поле
`.p_filesz`, отвечающим за размер сегмента в файле, и не нулевом `.p_memsz`, отвечающим за размер после загрузки.

```c++
const Elf64_Phdr BSS_PHEADER = {
        .p_type   = PT_LOAD,
        .p_flags  = PF_R | PF_W,
        .p_offset = 0,              /* (bytes into file) */
        .p_vaddr  = 0x404000,       /* (virtual addr at runtime) */
        .p_paddr  = 0x404000,       /* (physical addr at runtime) */
        .p_filesz = 0,              /* (bytes in file) */
        .p_memsz  = x64::RAMSIZE,   /* (bytes in mem at runtime) */
        .p_align  = 4096,           /* (min mem alignment in bytes) */
};
```

### Стандартная библиотека

[//]: # (Тут про процесс загрузки стандартной библиотеки)

### Сравнение времени работы

[//]: # (Сравнить времена работы лол)