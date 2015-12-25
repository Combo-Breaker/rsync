# отчет
Саркисян Вероника, 143.

Целью данного проекта была реализация утилиты rsync, синхронизирующей множество файлов между двумя серверами.
http://wiki.cs.hse.ru/%D0%9A%D0%A1:2015:%D0%9F%D1%80%D0%BE%D0%B5%D0%BA%D1%82:rsync

К моменту написания данного отчета мною была написана утилита, выводящая список команд (cp - копировать файл, rm - удалить файл), необходимых для синхронизации двух директорий на одном сервере.

Для реализации параллельных процессов использовались нити (threads), соответствующие клиенту и серверу. Нити обменивались данными через сокеты следующим образом: клиент посылает запрос на список файлов, сервер получив запрос отправляет клиенту файлы, тот вычисляет разницу и выводит команды на поток вывода. За синхронизацию работы сервера и клиента отвечают методы класса Protocol, за соединение - класс SocketConnection. Данные между сокетами отправляются в виде фреймов, сериализованных с помощью библиотеки boost::serialization. 
Для обработки исключений использован метод throw runtime_error(string s), вызываемый после каждого системного вызова. Кроме того, в ходе работы над проектом я научилась пользоваться отладчиком gdb и утилитой CMake.

Пример запуска: ./main ./dir1 ./dir2

где ./dir1 - абсолютный путь к синхронизируемой папке "клиента", ./dir2 - к папке "сервера".

