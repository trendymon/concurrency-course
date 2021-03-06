# Корутины

[_Корутина_](https://www.boost.org/doc/libs/1_60_0/libs/coroutine/doc/html/coroutine/intro.html) (или _сопрограмма_, _coroutine_) – это функция, из вызова которой можно выйти в середине, а затем вернуться в него и продолжить исполнение.

Чуть аккуратнее, корутина – это объект, который представляет собой вычисление, которое может останавливаться (по собственной воле) и возобновляться (по воле вызывающего кода или внешнего события).

## Поведение

Создадим корутину:

```cpp
Coroutine co(routine);
```

Здесь `routine` – произвольная пользовательская функция, которая будет исполняться корутиной `co`. Непосредственно создание корутины не приводит к запуску функции `routine`.

Созданная корутина запускается вызовом `co.Resume()`. После этого управление передается функции `routine`, и та исполняется до первого вызова `Suspend()` (или до своего завершения).

Вызов `Suspend()` в корутине останавливает ее исполнение, передает управление обратно caller-у и завершает его вызов `co.Resume()`. Вызов `Suspend()` – это точка выхода из корутины, _suspension point_.

Следующий вызов `co.Resume()` вернет управление остановленной корутине, вызов `Suspend()` в ней завершится, и она продолжит исполнение до очередного `Suspend()` или же до инструкции `ret`.

Код, исполняемый внутри корутины, не имеет доступа к самому объекту `Coroutine`. Чтобы остановить исполнение, корутина вызывает статический метод `Suspend`.

Для лучшего понимания API и потока управления в корутинах изучите [тесты](tests/coroutine.cpp) к задаче.

## Терминология

Есть мнение, что говорить _корутина_ – неграмотно, вместо этого следует использовать термин _сопрограмма_, по аналогии с _подпрограммой_ ([subroutine](https://en.wikipedia.org/wiki/Subroutine)).

Выберите вариант по душе.

## Виды корутин

В этой задаче мы говорим про конкретный вид корутин – [_stackful_](https://www.boost.org/doc/libs/1_60_0/libs/coroutine/doc/html/coroutine/intro.html#coroutine.intro.stackfulness) [_asymmetric_](https://www.boost.org/doc/libs/1_60_0/libs/coroutine/doc/html/coroutine/intro.html#coroutine.intro.execution_transfer_mechanism).

### Корутины в C++

[Корутины в C++20](https://en.cppreference.com/w/cpp/language/coroutines) – _stackless_ и реализованы на уровне языка, т.е. непосредственно в компиляторе, а не в виде библиотеки.

## Корутины и файберы

И stackful корутины, и файберы описывают вычисления, которые можно остановить, а затем – возобновить.
Обе сущности используют механизм переключения контекста для нелокальной передачи управления.

Но стоит отличать их друг от друга!

### Файберы

Файберы – это кооперативная многозадачность: вычисления исполняются конкурентно, синхронизируясь друг с другом.

За исполнение файберов отвечает планировщик, его задача – распределять файберы между потоками (аналогично планировщику операционной системы, который распределяет потоки между ядрами процессора).

Помимо планировщика файберам нужны собственные средства синхронизации: мьютексы, кондвары, каналы и т.д.

### Корутины

Корутины гораздо ближе к обычным функциям, чем к файберам.

Прямого отношения к конкурентности корутины не имеют. Корутины не нуждаются в примитивах синхронизации. У корутин нет планировщика и очереди.

Управление передается от caller-а к callee и обратно через вызовы `Resume` и `Suspend`, как при вызове обычных функций.

С исключениями корутины взаимодействуют тоже как обычные функции: если в корутине было выброшено и не перехвачено исключение, то оно вылетит в caller-е из вызова `co.Resume()` и полетит дальше (выше) по цепочке вызовов.

---

Чтобы окончательно разобраться в вопросе, прочтите [Distinguishing coroutines and fibers](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2014/n4024.pdf).

## От корутин к файберам

В этой задаче вы должны выразить файберы через корутины.

Вам уже дан готовый многопоточный планировщик – _пул потоков_ (_thread pool_).

## Пул потоков

Пул потоков ничего не знает про корутины и файберы, он исполняет _задачи_ (_tasks_) – `std::function<void()>`.

### Примеры

```cpp
// Создаем пул из 4-х потоков
StaticThreadPool pool{/*threads=*/4};

// Задача будет запущена в одном из 4-х потоков пула
// Вызов `Submit` вернет управление без ожидания
pool.Submit([]() {
  std::cout << "Hello from pool!" << std::endl;
});
```

Задачу в пул можно добавить не только снаружи, но и прямо из исполняемой задачи, т.е. из потока-воркера внутри пула:

```cpp
auto task = []() {
  // Находим текущий пул и кладем в него задачу
  StaticThreadPool::Current()->Submit(
    []() {
      std::cout << "We need to go depeer" << std::endl;
    }
  )
};

// Бросаем в пул внешнюю задачу
pool.Submit(task);
```

### Continuations

Если задача _A_ кладет задачу _B_ в пул, и при этом _B_ является _продолжением_ (_continuation_) _A_, то вместо `Submit` стоит использовать `SubmitContinuation`.

`SubmitContinuation` – это указание пулу, что задачу можно запланировать на исполнение только _после_ завершения текущей задачи.

### Join

Пул останавливается с помощью метода `Join`. После вызова `Join` потоки-воркеры будут работать до тех пор, пока в пуле не закончатся задачи. Вызов `Join` вернет управление после завершения всех потоков пула.

### Реализация

[`StaticThreadPool`](mtf/thread_pool/static_thread_pool.hpp)

Пул потоков реализован с помощью библиотеки [`asio`](https://github.com/chriskohlhoff/asio).

## Файберы

```cpp
// Создаем планировщик - пул потоков
StaticThreadPool scheduler{/*threads=*/4};

auto child = []() {
  // Уступаем текущий поток пула другому файберу
  Yield();
};

auto parent = []() {
  // Запускаем новый файбер внутри текущего пула
  Spawn(child);
};

// Запускаем новый файбер в пуле `thread_pool`
Spawn(parent, scheduler);

// Дожидаемся завершения файберов и останавливаем пул
scheduler.Join();
```

## Декомпозиция

Реализация многопоточных файберов свелась к комбинированию двух ортогональных составляющих:

- Пул потоков
- Stackful корутина

Пул потоков ничего не знает про природу задач, которые он исполняет, а корутины в свою очередь ничего не знают про многопоточность и concurrency. 

## Корутины и асинхронность

Корутины – базовый механизм, с помощью которого асинхронность реализуется в современных языках программирования.

Сами корутины реализуются на уровне языка, компилятором, а интеграция с асинхронными фреймворками и планировщиками осуществляется на уровне библиотек.

### C++

- [Гор Нишанов, C++ Coroutines – a negative overhead abstraction](https://www.youtube.com/watch?v=Ts-1mWBmTNE)
- https://github.com/lewissbaker/cppcoro

### Java

- [Project Loom with Ron Pressler and Alan Bateman](https://www.youtube.com/watch?v=J31o0ZMQEnI)
- [Project Loom: Fibers and Continuations for the Java Virtual Machine](http://cr.openjdk.java.net/~rpressler/loom/Loom-Proposal.html)
- [ForkJoinPool](https://docs.oracle.com/javase/8/docs/api/java/util/concurrent/ForkJoinPool.html) – планировщик

### Kotlin

- [Kotlin Coroutines Proposal](https://github.com/Kotlin/KEEP/blob/master/proposals/coroutines.md)
- [KotlinConf 2017 - Introduction to Coroutines by Roman Elizarov](https://www.youtube.com/watch?v=_hfBv0a09Jc)

## Генераторы и итераторы

Есть еще один зверь, похожий на корутины – [генераторы](http://www.dabeaz.com/generators/).

Корутины / генераторы изменят ваш взгляд на реализацию итераторов: [Iteration Inside and Out](https://journal.stuffwithstuff.com/2013/01/13/iteration-inside-and-out/)

## Задание

1) С помощью `ExecutionContext` реализуйте stackful корутину.

2) Через корутины и пул потоков выразите многопоточные файберы.

## Замечания по реализации

### Стеки

Класс `impl::Coroutine` получает лишь _view_ стека, он сам не аллоцирует и не управляет памятью.

За управление ресурсами отвечает код уровнем выше.

Например, `SimpleCoroutine`, которая используется в тестах, просто аллоцирует новый стек в конструкторе и освобождает память в деструкторе.

Для файберов потребуется более эффективный механизм. Реализуйте простой пул стеков, защищенный спинлоком. В будущем мы узнаем, как написать для пула _lock-free_ реализацию.

Не аллоцируйте память под спинлоком. Спинлоки годятся только для очень простых и коротких критических секций.

#### Пул

Аналогично [TinyFibers](https://gitlab.com/Lipovsky/tinyfibers/-/blob/master/tinyfibers/runtime/stacks.cpp) весь код аллокации стеков должен быть спрятан от файберов за функциями `AcquireStack` / `ReleaseStack`.

#### Спинлок

Реализуйте _Test-and-TAS_ спинлок, который учитывает когерентность кэшей.

Реализацию поместите в отдельный заголовочный файл.

Для совместимости с `std::lock_guard` в спинлоке потребуется написать методы с именами `lock` и `unlock`, что не укладывается в стайл-гайд. См. [Suppressing Undesired Diagnostics](https://clang.llvm.org/extra/clang-tidy/#suppressing-undesired-diagnostics) для подавления проверок `clang-tidy`.

### `Submit` / `SubmitContinuation`

Используйте метод `SubmitContinuation` у `StaticThreadPool` только если вам недостаточно метода `Submit`.

### Исключения

Используйте [std::exception_ptr](https://en.cppreference.com/w/cpp/error/exception_ptr) для прокидывания исключения из корутины в caller-а.

### `ExecutionContext`

Финализируте с помощью вызова `AfterStart` первое переключение контекста, которое, в отличие от последующих, прыгает не из `SwitchTo` в другой `SwitchTo`, а в трамплин корутины.

См. [Fiber::Trampoline](https://gitlab.com/Lipovsky/tinyfibers/-/blob/7e0397fe400f5b8f52eb805913a30764415c52f4/tinyfibers/runtime/fiber.cpp#L25) для примера.

### Misc

Не добавляйте в публичный интерфейс класса `Coroutine` служебные методы, которые пользователь корутины не должен вызывать напрямую.
