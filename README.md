# Сервис дедупликации данных

Цель работы состоит в реализации системы оптимального хранения
данных за счет использования подхода дедупликации данных и проведении
тестирования для измерения производительности созданного прототипа.

## Задачи

- [x] решить проблему с подключением к бд
- [x] создать простую схему для дедупликации
- [ ] Доработать данную схему базы данных
  - [ ] Добавить методы для сбора метрик(процент сжатия, распределение по числу блоков)
  - [x] Убрать ненужные/избыточные колонки, нормализовать/уточнить взаимосвязи между таблицами
  - [x] добавить методы блочной передачи, чтобы передавать не сразу весь файл, а по фрагментам
  - [x] Реализовать процедуры удаления файлов из бд
- [ ] реализовать создание схемы выше в разных вариантах(размер сегмента, функция хэширования) при помощи динамических sql запросов
  - [x] Реализовать процедуру создания базы данных
  - [x] Реализовать процедуру заполнения схем новой бд
  - [ ] добавить возможность выбирать функцию хэширования
- [ ] добавить источник данных
  - [x] Источник данных директория(любая)
  - [ ] Подобрать данные для проверки работы сервиса
- [x] реализовать сервис для сбора данных из директорий/архивов и их восстановления, внутри будет использован dbService
  - [ ] Доработать набор запросов для взаимодействия с файлами/директориями
  - [x] Доделать реализации для стратегий работы с бд
- [x] определить стек технологий
  - [ ] Доработать стек ниже
- [ ] Ужать логи для запросов, стандартизировать их содержимое
  - [x] Ввести единое соответствие между кодами возврата и типом ошибки
  - [x] Убрать лишние(\n,\r\n ...) символы из записей логгера
  - [ ] Настроить стандартное место для записи логов
  - [ ] Убрать тексты запросов из записей
- [x] Произвести миграцию со старой версии libpqxx на новую
- [ ] Создать набор тестов для проверки работы сервиса
  - [ ]CI/CD?
- [ ] ввести набор критериев и метрик, зависимости между которыми будут подлежать исследованию
## Стек технологий
1. C++ 20
2. libpqxx
3. libglog
4. libopenssl
5. libopencv
6. gtest
7. gmock
## Wsl firewall set-up

[source](https://stackoverflow.com/questions/56824788/how-to-connect-to-windows-postgres-database-from-wsl)

WSL2 assigns IP address to the Windows host dynamically and the IP addresses can change without even rebooting Windows (
see Notes below). So to reliably connect we'll need to:

1. Allow Windows and Postgres to accept connections from the WSL2 IP address range (not allowed by default)
2. From WSL2, determine the Windows/Postgresql host's IP address (which is dynamic) when connecting via`psql`. We'll
   make this convenient via`.bashrc`and`alias`.

Unfortunately I couldn't find the exact specification for the WSL2 IP address range. From several tests/reboots it
appears that WSL2 is assigning IP addresses primarily in range of`172.*.*.*`but I have occasionally been
assigned`192.*.*.*`so we'll use these when configuring the firewall and Postgres.

**Add Windows Firewall Inbound Port Rule for WSL2 IP Addresses:**

1. Open `Windows Defender Firewall with Advanced Security`
2. Click `New Rule...`
3. Select `Port` for rule type
4. Select `TCP` and for `Specific local ports` enter `5432`
5. Select `Allow the connection`. Connecting from WSL2 won't be secure so don't select the secure option
6. Select at least `Public`. Can select `Domain` and `Private` as well. I could only connect if `Public` was selected
7. Name the rule e.g. `Postgres - connect from WSL2` and create it
8. Right click newly created rule and select `Properties` then click on the `Scope` tab
9. Under`Remote IP address`, select`These IP addresses`then click`Add...`and enter range`172.0.0.1` to `172.254.254.254`
10. Repeat step 9 for IP address range`192.0.0.1`to`192.254.254.254`
11. Click`Apply`then`OK`
12. Make sure rule is enabled
