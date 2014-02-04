#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTcpSocket>
#include <QtGui>
#include <QMessageBox>
#include <QTextCodec>
#include <QList>
#include <QBrush>
#include <QListWidgetItem>
#include <QTimer>
#include <QProcess>
#include <QSignalMapper>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();



private slots:
    void initialapp(); // чтение настроек
    void set_address(); // запись настроек ip
    void set_port (); // запись настроек порта
    void connectForSwitching (QString states); // подключения для включения, передаем значения каждого канала на мохе, тут же формируется команда для мохи
    void switching (); // включает крн
    void reset (); // Сбрасывает управляемые каналы в 0
    void bad_connection(); // Выводит сообщение о слишком долгом выполнении команды и закрывает соединения


private:
    QSignalMapper *signalMapper; // для передачи параметров
    QSettings krn_settings; // настройки приложения
    QByteArray command; // команда, будет формироваться в теле слота подключения сокета к мохе
    QTimer *control_timer; // Таймер для контроля управления КРН, если после отправки команды пройдет слишком много времени, то будет выведено соответсвующее сообщение
    QList<QString> res_dio; // ответ от MOXA
    QString ip; // адрес подключения
    quint16 port; // порт подключения
    QTcpSocket *sock; // сокет

    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
