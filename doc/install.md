[На страницу с документацией](README.md)
# Инструкция по работе и установке
## Про конфигурацию 
При установке репозитория в нём должна существовать папка 
conf cо следующей конфигурацией(conf/configuration.txt):
```
testuser
password
postgres
5432
```
Данная конфигурация подойдёт в случае запуска приложения из docker контейнера.

Однако, если код собирается на месте, то следует использовать данный вариант:
```
testuser
password
localhost
5501
```
## Установка и сборка исходного кода
### Сборка на toolchain рабочего устройства
```bash
git clone https://github.com/AlexanderMeynik/data_deduplication_service.git
mkdir build &&cd build
cmake ..
#cmake -DBUILD_DOC=ON -DBUILD_GUI=ON .. #to build ui and documentation
```
### Сборка с использованием docker образа

```bash
git clone https://github.com/AlexanderMeynik/data_deduplication_service.git
docker compose run --rm  service
mkdir build &&cd build
cmake ..
#cmake -DBUILD_DOC=ON -DBUILD_GUI=ON .. #to build ui and documentation
```
## Запуск тестов
```bash
cd test
./clock_array_tests
./FileServiceTests
```
## Запуск бэнчмарка
```bash
#download test fixture
wget https://github.com/AlexanderMeynik/data_deduplication_service/releases/download/fixture/testDirectories.zip
unzip testDirectories.zip
rm testDirectories.zip
cd test
./benchmark
```

## Сервис PostgreSQL
Для запуска сервиса достаточно в корневой директории выполнить.

```bash
docker-compose up -d postgres
```

[На страницу с документацией](README.md)