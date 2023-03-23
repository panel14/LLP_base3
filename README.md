# LLP_base3

### Лабораторная работа №3 по низкоуровневому программированию

Вариант - ProtocolBuffers

## Сборка приложения
`sh build.sh`

## Запуск приложения

Сервер

`./serv <flag> <storagefile> [<datafile>]`

### Пример
`./serv -p storage.txt dg/registry.txt`

Клиент

`./client <host>`

### Пример
`./client 127.0.0.1`
