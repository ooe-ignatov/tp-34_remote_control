#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <qtcpsocket.h>
#include <QString>
#include <QLineEdit>
#include <QSettings>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    krn_settings("OOE", "TP-34"),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    QTextCodec* codec = QTextCodec::codecForName("UTF8"); // указываем кодировку
    QTextCodec::setCodecForTr(codec);
    QTextCodec::setCodecForCStrings(codec);
    QTextCodec::setCodecForLocale(codec);

    this->move(QApplication::desktop()->availableGeometry().topLeft()); // запуск окна в левом верхнем углу
    //создаем сокет
    sock = new QTcpSocket(this); // сокет для опроса

    ui->ip_label->setText(ip);

    command.resize(14); // записываем в массив команду. Описание команды см в инструкции к MOXA 4100
    command[0]=06;
    command[1]=02;
    command[2]=00;
    command[3]=10;
    command[4]=00; // номер стартового канала 0, 1, 2, 3
    command[5]=03; // номер последнего канала
    command[6]=01; // режим первого канала вх/вых
    command[7]=01; // состояние первого канала
    command[8]=01;
    command[9]=01;
    command[10]=01;
    command[11]=01;
    command[12]=01;
    command[13]=01;

    //теперь попробуем signalMapper
    signalMapper = new QSignalMapper(this);
    signalMapper ->setMapping(ui->onBut_1, QString("0,1,1,1"));
    signalMapper ->setMapping(ui->offBut_1, QString("1,0,1,1"));
    signalMapper ->setMapping(ui->onBut_2, QString("0,0,1,1"));
    signalMapper ->setMapping(ui->offBut_2, QString("1,1,0,1"));
    signalMapper ->setMapping(ui->onBut_3, QString("0,1,0,1"));
    signalMapper ->setMapping(ui->offBut_3, QString("1,0,0,1"));
    signalMapper ->setMapping(ui->onBut_4, QString("0,0,0,1"));
    signalMapper ->setMapping(ui->offBut_4, QString("1,1,1,0"));

    connect(ui->onBut_1, SIGNAL(clicked()), signalMapper, SLOT(map()));
    connect(ui->offBut_1, SIGNAL(clicked()), signalMapper, SLOT(map()));
    connect(ui->onBut_2, SIGNAL(clicked()), signalMapper, SLOT(map()));
    connect(ui->offBut_2, SIGNAL(clicked()), signalMapper, SLOT(map()));
    connect(ui->onBut_3, SIGNAL(clicked()), signalMapper, SLOT(map()));
    connect(ui->offBut_3, SIGNAL(clicked()), signalMapper, SLOT(map()));
    connect(ui->onBut_4, SIGNAL(clicked()), signalMapper, SLOT(map()));
    connect(ui->offBut_4, SIGNAL(clicked()), signalMapper, SLOT(map()));

    connect(signalMapper, SIGNAL(mapped(QString)), this, SLOT(connectForSwitching(QString)));






    //соединяем сигналы и прочее
    control_timer = new QTimer;
    //connect(ui->offBut_1, SIGNAL(clicked()), this, SLOT(connectForSwitching(QList<int>)));
    //connect(ui->onBut_1, SIGNAL(clicked()), this, SLOT(switching()));
    connect(sock, SIGNAL(connected()), this, SLOT(switching()));
    connect(sock, SIGNAL(readyRead()), this, SLOT(reset()));
    connect(control_timer, SIGNAL(timeout()), this, SLOT(bad_connection()));
    connect(ui->Address, SIGNAL(triggered()), this, SLOT(set_address()));
    connect(ui->Port, SIGNAL(triggered()), this, SLOT(set_port()));

    initialapp();;

}

/********************** Инициализация приложения *************************/
void MainWindow::initialapp()
{
    if ((!krn_settings.value("/settings/address").isNull()) && (!krn_settings.value("/settings/port").isNull()))
    {
        ip = krn_settings.value("/settings/address").toString();
        port = krn_settings.value("/settings/port").toString().toShort();
        ui->ip_label->setText(ip);
    }
    else
    {
        ip = "Нет настроек подключения!";
        ui->ip_label->setText(ip);
    }
}

/********************* Запись настроек IP *********************/
void MainWindow::set_address()
{
    bool set_ip;
    QString str = QInputDialog::getText(this, "Введите адрес", "Адрес:", QLineEdit::Normal, "", &set_ip);

    if (!set_ip)
    {
        return;
    }
    else
    {
        krn_settings.setValue("/settings/address", str);
        initialapp();
    }
}

/**************** Запись настроек порта *******************/
void MainWindow::set_port()
{
    bool set_port;
    QString str = QInputDialog::getText(this, "Введите порт", "Порт:", QLineEdit::Normal, "", &set_port);

    if (!set_port)
    {
        return;
    }
    else
    {
        krn_settings.setValue("/settings/port", str);
        initialapp();
    }
}


/************************* ПОДКЛЮЧЕНИЕ ДЛЯ ВКЛЮЧЕНИЯ **********************/
void MainWindow::connectForSwitching(QString states)
{
    QStringList statesList =  states.split(",");
    command[7] = statesList.at(0).toInt();
    command[9] = statesList.at(1).toInt();
    command[11] = statesList.at(2).toInt();
    command[13] = statesList.at(3).toInt();

    //qDebug() << c0 << c1 << c2 << c3;
    sock->connectToHost(ip, port);
}


/********************************** ОТПРАВКА КОМАНДЫ *************************/
void MainWindow::switching()
{
    int reaction; // Используется для вывода сообщения об ошибке и нажатии определенной кнопки в этом сообщении.
    bool pas;
    QString str = QInputDialog::getText( 0, "Password", "Пароль:", QLineEdit::Password,"" ,&pas);

    if (!pas)
    {
        sock->close();

        qDebug() << "Нажали кнопку, и отмену на вводе пароля";
    }
    else
    {
        if (str=="123456")
        {
            reaction = QMessageBox::warning(this, "Подтвердите действие",
            "Вы уверены?", "Да", "Отмена", QString(), 0, 1); // Вывод сообщения для подтверждения действия.
            if(!reaction) // Условие для определения нажатой кнопки.
            {
            control_timer->start(30000); // если за 30 сек не выполнится вся цепочка комманд, то диспетчер будет оповещен об этом.

            sock->write(command); // отправка

            qDebug() << "отправка команды";
            }
        }
        else
        {
            QMessageBox::information(this, "Неверный пароль!", "Вы ввели неверный пароль!");
            sock->close();
            qDebug() << "Был введен неверный пароль при попытке включения";
        }
    }
}



/*************************** СБРОС УПРАВЛЯЕМЫХ КАНАЛОВ ******************/
void MainWindow::reset()
{

    QList<QString> res_management; // ответ от MOXA на попытку управления, пригодится при отладке
    QByteArray set_dio; // команда сброса
    set_dio.resize(14); // записываем в массив команду. Описание команды см в инструкции к MOXA 4100

    res_management.clear(); // очищаем результат ответа


        QEventLoop loop;
        QTimer::singleShot(2000, &loop, SLOT(quit()));
        loop.exec();

    int i=sock->bytesAvailable(); // количество байт, которое будет принято
    int j; // счетчик
    for (j=0; j<i; j++)
    {
        res_management << sock->read(1).toHex(); // Пишем в список побайтно ответ от MOXA
    }


    qDebug() << "ответ на попытку управления: " << res_management; // ответ от мохи, может помочь при отладке

    set_dio.resize(14); // записываем в массив команду. Описание команды см в инструкции к MOXA 4100
    set_dio[0]=06;
    set_dio[1]=02;
    set_dio[2]=00;
    set_dio[3]=10;
    set_dio[4]=00; // номер стартового канала 0, 1, 2, 3
    set_dio[5]=03; // номер последнего канала
    set_dio[6]=01; // режим первого канала вх/вых
    set_dio[7]=01; // состояние первого канала
    set_dio[8]=01;
    set_dio[9]=01;
    set_dio[10]=01;
    set_dio[11]=01;
    set_dio[12]=01;
    set_dio[13]=01;

    sock->write(set_dio);

    sock->close();

    control_timer->stop();

    qDebug() << "Socket close";

}

/***************************** ВЫВОД СООБЩЕНИЯ ОБ ОШИБКЕ УПРАВЛЕНИЯ ****************************/
void MainWindow::bad_connection()
{
    QMessageBox::information(this, "Что-то не так!", "С момента отправки команды прошло слишком много времени! \n Попробуйте еще раз, если опять увидите это сообщение, то необходимо выехать на объект");

    control_timer->stop();

    sock->close();
    qDebug() << "вероятно что-то не так со связью, сокет для включения закрыт";


}

MainWindow::~MainWindow()
{
    delete ui;
}
