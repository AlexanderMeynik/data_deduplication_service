[На страницу с документацией](doc_main.md)

Для удобной настройки и размещения релизов был создан
отдельный [репозиторий](https://github.com/AlexanderMeynik/docker-packaging/tree/latest).

# Контейнерезированное приложение

Для работы с данным релизом потребуется следующее ПО:

1. Docker/docker-compose верссии >= 2.15.1
2. Сервер XORG: [Xlaunch](http://www.straightrunning.com/xmingnotes/IDH_PROGRAM.htm) для
   Windows, [Xquarts](https://www.xquartz.org/index.html) for для Mac
   или [ServerGui](https://help.ubuntu.com/community/ServerGUI) для Linux.

## Запуск с использованием готового релиза

Для установки и запуска приложения достаточно выполнить следующий набор действий:

Установить последний [релиз](https://github.com/AlexanderMeynik/docker-packaging/releases/tag/latest) данного
приложения.

Распаковать данный архив:

```bash
unzip Distribution.zip
cd Distribution
```

Выполнить установку образов:

```bash
sudo docker compose pull
```

Запустить выбранную версию сервера XORG в режиме множества окон с опцией "No Access Control".

Запустить приложение можно командой:

```bash
sudo docker compose up -d
```

Информация из бд сервреа будет храниться в docker volume ...h2data.

Завершить работу приложения:

```bash
sudo docker compose stop
```

## Самостоятельная сборка образов

Исходный код приложения можно установить из релиза:
[Ссылка для скачивания](https://github.com/AlexanderMeynik/docker-packaging/archive/refs/tags/latest.tar.gz).
И далее распаковать полученный архив:

```bash
tar -xzf docker-packaging-latest.tar.gz
cd docker-packaging-latest
```

Дальнейшия действия с кодом схожи с таковыми для готовго релиза, только вместо pull в 1 команде образы мы будем
собирать:

```bash
sudo docker compose build
```

# Запуск приложения с иcпользованием maven

Для осуществления данной процедуры потребуется Java 19 установленная вместе с паектным менеджером maven.

После установки исходного кода следует открыть файл:

```bash
nano backend/src/main/resources/application.yaml
```

И закомментировать там 3 строчку при этом следует раскомментировать строчку 4 и сохранить изменения.

Собрать все файлы проекта:

```bash
mvn install -DskipTests
```

Запустить сервер:

```bash
mvn spring-boot:run -pl backend 
```

Запустить клиент:

```bash
mvn javafx:run -pl client
```

