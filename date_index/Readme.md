# DateIndex — Bit-Slice Indexing для дат

## Описание

`DateIndex` — индекс документов с поддержкой дат начала/окончания жизни и поиска по диапазонам дат с использованием **bit-slice indexing**.

### Bit-Slice Indexing

Каждая дата преобразуется в целое число (дни от 1970-01-01). Для каждого бита числа создаётся виртуальный термин в отдельном LSM-хранилище:

- `begin0`, `begin1`, ..., `begin20` — биты даты начала
- `end0`, `end1`, ..., `end20` — биты даты окончания

Если дата окончания не задана, используется максимальное значение `(2^21 - 1)`, что означает бессрочный документ.

Запросы по диапазонам дат `[lo, hi]` преобразуются в AND/OR формулы над битовыми срезами:

- **value >= lo**: для каждого бита от старшего к младшему определяем, какие документы точно больше (GT) и какие пока равны (EQ)
- **value <= hi**: аналогично, но определяем LT и EQ
- **lo <= value <= hi**: пересечение `value >= lo` и `value <= hi`

### Поддерживаемые запросы

- **Текстовый поиск**: `cat AND dog`, `NOT mouse`, `(apple OR banana) AND cherry`
- **DATE_RANGE(lo, hi)**: документы, валидные в диапазоне дат (begin ≤ hi AND (end ≥ lo OR нет end))
- **CREATED_IN(lo, hi)**: документы, появившиеся в диапазоне дат (lo ≤ begin ≤ hi)
- **Комбинированные**: `cat AND DATE_RANGE(2024-01-01, 2024-12-31)`

## Run unit tests

```bash
g++ -std=c++17 -Wall -Wextra -Wno-unused-parameter date_index.cpp ../inverted_index/inverted_index.cpp ../common/roaring/roaring.c test_date_index.cpp -o tests -I/opt/homebrew/Cellar/googletest/1.17.0/include -L/opt/homebrew/Cellar/googletest/1.17.0/lib -lgtest -lgtest_main && ./tests
```
