# Atomic

Реализуйте класс `stdlike::Atomic` для x86-64, который поддерживает различные режимы упорядочивания памяти:

- `Relaxed`
- `Release` / `Acquire`
- `SeqCst`

## Пример

```cpp
using stdlike::Atomic;
using stdlike::MemoryOrder;

Atomic test{0};

test.Store(1, MemoryOrder::Release);

test.Load(MemoryOrder::SeqCst);
// Упорядочивание по умолчанию - MemoryOrder::SeqCst
test.Load();
```

## Полезные ссылки

- https://godbolt.org/ + обязательно установите флаг `-O2`.