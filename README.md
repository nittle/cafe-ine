# Кафе-Ин

Приложение для заказа блюд из кафе

## Функционал:

- Меню (наименование, вес/размер, состав, цена, картинка)
- Корзина для добавления и заказа блюд
- Профиль (история заказов, баллы)
- О нас(написать отзыв)
- Доставка

## Сборка

### Linux

- Установить wxWidgets (pacman) `sudo pacman -S wxwidgets-gtk3`
- Настроить папку для сборки `cmake -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON`
- Собрать `cmake --build build`

Исполняемый файл: `build/cafe-in`

### Visual Studio (Windows)

- Скачать *Header Files* и *Development Files* с сайта [wxWidgets](https://wxwidgets.org/downloads/), кнопка *Download Windows Binaries*
- Распаковать оба архива в папку для библиотеки(например, `C:\Libraries\wxWidgets`)
- Добавить папку в переменную окружающей среды `setx wxWidgets_ROOT "C:\Libraries\wxWidgets"`
- Перезапустить Visual Studio и CMake автоматически увидит `wxWidgets_ROOT`
Распределение задач:
  Даниил Друк: тим лид и логика программы;
  Бармин Стефан: база данных;
  Садовская Дарья: GUI;
