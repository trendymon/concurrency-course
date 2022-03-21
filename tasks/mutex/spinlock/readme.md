# SpinLock

В этой задаче мы узнаем из чего сделаны [атомики](https://en.cppreference.com/w/cpp/atomic/atomic).

Вам дана [реализация простейшего Test-and-Set спинлока](spinlock.hpp).

Но в ней не хватает реализации атомарных операций – `AtomicStore` и `AtomicExchange`. 

Семантика этих операций точно такая же, как у `store` и `exchange` в атомиках из стандартной библиотеки.

Реализуйте их!

А заодно заполните реализацию метода `TryLock` у спинлока.

## Asm 101

Реализовать атомики в языке невозможно, придется писать на ассемблере. Реализация получится платформо-зависимой, в нашем случае это x86-64.

Заготовки для функций вы найдете в файле [atomic_ops.S](atomic_ops.S). 

Вспомните / изучите calling conventions – [System V AMD64 ABI](https://en.wikipedia.org/wiki/X86_calling_conventions#System_V_AMD64_ABI):
- Функции получают первые два аргумента через регистры `%rdi` и `%rsi`.
- Возвращают результат (если он есть) через регистр `%rax`.

Для справки по ассемблеру см. [Introduction to X86-64 Assembly for Compiler Writers](https://web.archive.org/web/20160714182232/https://www3.nd.edu/~dthain/courses/cse40243/fall2015/intel-intro.html)

## `AtomicStore` против `atomic<T>::store`

У методов атомиков в стандартной библиотеке есть дополнительный параметр: [`std::memory_order`](https://en.cppreference.com/w/cpp/atomic/memory_order).

В `store` разумно передавать лишь некоторые из них: `relaxed`, `release` и `seq_cst`.

Каждому из этих вариантов в общем случае соответствует своя собственная реализация метода `store`.

### Вопросы

- `store` для какого `std::memory_order` вы написали в функции `AtomicStore`?
- А какая версия нужна спинлоку?

## Полезные ссылки

- https://godbolt.org/ + обязательно установите флаг `-O2`.
