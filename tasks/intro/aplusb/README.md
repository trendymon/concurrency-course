#  A+B

Реализуйте метод `ComputeAnswer` класса `DeepThought` в файле [`deep_thought.hpp`](deep_thought.hpp).

Метод должен вычислять [ответ на главный вопрос жизни, Вселенной и всего остального](https://en.wikipedia.org/wiki/42_(number)#The_Hitchhiker's_Guide_to_the_Galaxy). 

## Как работать с задачей

Для запуска тестов выполните в директории с задачей команду `clippy test`.

Для запуска конкретной тестовой цели выполните команду `clippy target {target-name} -p {build-profile-name}`.

Для автоматического форматирования кода задачи с помощью [`clang-format`](https://clang.llvm.org/docs/ClangFormat.html) выполните команду `clippy format`. Конфиг для `clang-format` находится в корне репозитория в файле [`.clang-format`](/.clang-format).

Для проверки кода с помощью линтера [`clang-tidy`](https://clang.llvm.org/extra/clang-tidy/) выполните команду `clippy tidy`. Конфиг для `clang-tidy` находится в корне репозитория в файле [`.clang-tidy`](/.clang-tidy).

Разрешается менять только файлы / директории, перечисленные в поле `submit_files` конфига задачи [`task.json`](task.json). Только эти файлы, как следует из названия поля, будут отправлены на проверку.

В CI на решении запускается команда `clippy validate` для проверки качества кода и запрещенных паттернов.
