# Мьютекс

В этой задаче вы должны реализовать [std::mutex](https://ru.cppreference.com/w/cpp/thread/mutex).

Отличие мьютекса от спинлока в том, что мьютекс _блокирует_ поток на время ожидания.

## Futex

Реализовать блокировку потока операционной системы целиком в пространстве пользователя невозможно, остановить / запланировать поток может только планировщик операционной системы.

Для блокировок операционная система предоставляет _futex_ – ядерная очередь спящих потоков, которая привязана к ячейке памяти в адресном пространстве пользователя.

Пользователь работает с фьютексом через одноименный системный вызов – [futex(2)](http://man7.org/linux/man-pages/man2/futex.2.html).

### References

- [Basics of Futexes](https://eli.thegreenplace.net/2018/basics-of-futexes/)
- [futex(2)](http://man7.org/linux/man-pages/man2/futex.2.html)
- [kernel/futex.c](https://github.com/torvalds/linux/blob/master/kernel/futex.c)

### `twist::stdlike`

Наш атомик расширяет интерфейс атомика из `std` методами `FutexWait` и `FutexWakeOne` / `FutexWakeAll`, которые напрямую выполняют системный вызов `futex`.

Внимание: это не стандартные методы, у `std::atomic` их нет!

### Атомарность

Конкурирующие вызовы `FutexWait` и `FutexWake{One,All}` будут атомарны относительно друг друга: в ядре при работе с очередью фьютекса берется спинлок.

При этом вызов `FutexWait` не будет атомарным относительно `store`: запись в атомик может выполниться между проверкой условия и парковкой потока в `FutexWait`.


### `atomic::wait`

Начиная с С++20, с фьютексом можно работать через методы атомиков: [`wait`](https://en.cppreference.com/w/cpp/atomic/atomic/wait) + `notify_one` / `notify_all`.

Семантика `wait`:

```cpp
// https://eel.is/c++draft/atomics.types.generic#lib:atomic,wait
void wait(T old) {
  while (this->load() == old) {
    FutexWait(old);
  }   
}
```

Реализация `wait` подвержена [A-B-A problem](https://en.wikipedia.org/wiki/ABA_problem).

### Реализация в стандартной библиотеке

- [libc++](https://github.com/llvm/llvm-project/blob/main/libcxx/src/atomic.cpp)
- [libstdc++](https://github.com/gcc-mirror/gcc/blob/master/libstdc%2B%2B-v3/include/bits/atomic_wait.h)

### Ограничения

Фьютекс работает только с 32-битными словами в пространстве пользователя, так что используйте в реализации `atomic<uint32_t>`.

### Проверка в `atomic::wait`

Может показаться, что аргумент `old` и проверка в [`atomic::wait`](https://en.cppreference.com/w/cpp/atomic/atomic/wait) – избыточны, и было бы достаточно иметь `wait` без аргументов.

Проведите эксперимент: попробуйте решить задачу используя для блокирующего ожидания класс [`WaitQueue`](wait_queue.hpp) с методами `Park` (без аргументов) и `WakeOne` / `WakeAll`.


## Требования к реализации

* Захват и освобождение мьютекса, на владение которым больше никто не претендует, должен происходить максимально быстро, без переключения в ядро операционной системы.

* Если поток не может захватить блокировку, потому что его опережают другие потоки (такую ситуацию называют _contention_), то он должен _заблокироваться_ до освобождения мьютекса и освободить процессор.
