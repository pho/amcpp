#include "amcpp.h"
#include "ui_amcpp.h"
#include "configdialog.h"
#include <phonon/mediaobject.h>
#include <QList>
#include <QFileDialog>
#include <QDesktopServices>
#include <cstdlib>
#include <iostream>

amcpp::amcpp(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::amcpp)
{
    authToken = "noauth";
    lastSongIndex = -1;

    audioOutput = new Phonon::AudioOutput(Phonon::MusicCategory, this);

    mediaObject = new Phonon::MediaObject(this);
    mediaSource = new Phonon::MediaSource(QString("/home/pho/Music/Eluveitie/Slania/Inis Mona.mp3"));
    mediaObject->setCurrentSource(*mediaSource);

    Phonon::createPath(mediaObject, audioOutput);
    audioOutput->setVolume(0.5);

    ui->setupUi(this);

    ui->seekSlider->setMediaObject(mediaObject);
    ui->volumeSlider->setAudioOutput(audioOutput);

    amHandshake();

    connect(mediaObject, SIGNAL(finished()), this, SLOT(nextSong()));

 // connect(mediaObject, SIGNAL(totalTimeChanged(qint64)), ui->totalLabel, SLOT(setNum(int)));

    connect(mediaObject, SIGNAL(bufferStatus(int)), ui->progressBar, SLOT(setValue(int)));

    connect(mediaObject, SIGNAL(stateChanged(Phonon::State,Phonon::State)),
            this, SLOT(checkStatus(Phonon::State,Phonon::State)));

}

amcpp::~amcpp()
{
    delete ui;
}

void amcpp::changeEvent(QEvent *e)
{
    QMainWindow::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

void amcpp::on_playButton_clicked()
{
    if(mediaObject->state() == Phonon::PlayingState){
        pause();
    }
    else{
        play();
    }
}



void amcpp::amHandshake(){

    QCA::Initializer init;

    authToken = "noauth";

    if ( !QCA::isSupported("sha256") ){
        ui->statusBar->showMessage("sha256 is not supported. Exiting...\n");
        //burn da haus
        return;
    }
    else {
        ui->statusBar->showMessage("sha256 is supported :D\n");
        ui->statusBar->showMessage("Creating handshake\n");

        QSettings settings("amcpp", "amcpp");

        QString amurl = settings.value("amUrl").toString();
        QString user = settings.value("username").toString();
        QCA::SecureArray pass(settings.value("streamPass").toByteArray());


        //TODO Redo this with SecureArray

        char key[80];
        char timestamp[11];
        snprintf(timestamp, 11, "%ld", time(0));

        QString lol(QCA::Hash("sha256").hash(pass).toByteArray().toHex());

        snprintf(key, 80, "%s%s", timestamp, qPrintable(lol));

        QString passphrase = QCA::Hash("sha256").hashToString( key );

        QUrl url( QString("%3/server/xml.server.php?action=handshake&auth=%1&timestamp=%2&version=350001&user=%4").arg(qPrintable(passphrase), timestamp, amurl, user )) ;
        QNetworkRequest request(url);
        reply = manager.get(request);

        connect(reply, SIGNAL(readyRead()),
                this, SLOT(handshakeReply()));
    }
}

void amcpp::handshakeReply()
{

    QDomDocument e;
    e.setContent(reply->readAll());

    if (e.firstChildElement().firstChildElement().tagName() == "auth"){
        authToken = e.firstChildElement().firstChildElement().text();
        ui->statusBar->showMessage("Auth completed!");
    }
    else{
        ui->statusBar->showMessage("Auth invalid!!");
        //TODO Disable all
    }
}


void amcpp::on_searchButton_clicked()
{
    QSettings settings("amcpp", "amcpp");
    QString amurl = settings.value("amUrl").toString();

    ui->lineEdit->setDisabled(true);
    QUrl url( QString("%3/server/xml.server.php?action=search_songs&auth=%1&filter=%2").arg(authToken, ui->lineEdit->text(), amurl)) ;
    QNetworkRequest request(url);
    reply = manager.get(request);

    connect(reply, SIGNAL(finished()),
            this, SLOT(searchReply()));
}

void amcpp::searchReply()
{
    QDomDocument e;
    e.setContent(reply->readAll());

    ui->treeWidget->clear();

    QDomElement el = e.firstChildElement();

    for( int i = 0; i < el.childNodes().count(); i++ ){
        QDomElement song = el.childNodes().at(i).toElement();
        QString title, artist, url;

        for ( int j = 0; j < song.childNodes().count(); j++){
            QString tag = song.childNodes().at(j).toElement().tagName();

            if (tag =="title")
                title = song.childNodes().at(j).toElement().text();

            if (tag == "artist")
                artist = song.childNodes().at(j).toElement().text();

            if (tag == "url")
                url = song.childNodes().at(j).toElement().text();

        }
        QTreeWidgetItem *item = new QTreeWidgetItem;
        item->setText(0, title);
        item->setText(1, artist);
        item->setText(2, url);
        ui->treeWidget->addTopLevelItem(item);
    }

    lastSongIndex = -1;
    ui->lineEdit->setEnabled(true);
}

void amcpp::on_treeWidget_itemDoubleClicked(QTreeWidgetItem *item, int /*column*/)
{
    stop();

    currentTitle = item->text(0);
    currentArtist = item->text(1);
    currentUrl = item->text(2);

    lastSongIndex = ui->treeWidget->indexOfTopLevelItem(item);
    qDebug() << "Clicked index:"  << lastSongIndex;
    changeSong(item->text(2));

    play();

}

void amcpp::play(){
    if (lastSongIndex == -1)
        nextSong();
    else {
        mediaObject->play();
        ui->playButton->setText("Pause");
    }
}


void amcpp::pause(){
    mediaObject->pause();
    ui->playButton->setText("Play");
}

void amcpp::changeSong(QString str){

    delete mediaSource;

    mediaSource = new Phonon::MediaSource(str);
    mediaObject->setCurrentSource(*mediaSource);

    mediaObject->setTickInterval(1);

    qDebug() << "Next Song:" << str << "totaltime" <<  mediaObject->totalTime();

    ui->statusBar->clearMessage();
    ui->statusBar->showMessage(currentTitle + " - " + currentArtist);
    ui->titleLabel->setText(currentTitle + " - " + currentArtist);
    ui->totalLabel->setText(QString("%1").arg((float) mediaObject->totalTime()/60000));
    //ui->totalLabel->setTextFormat();

}

void amcpp::stop(){
    mediaObject->stop();
    ui->playButton->setText("Play");
}

void amcpp::nextSong(){

    if( ui->treeWidget->topLevelItemCount() > ++lastSongIndex){
        qDebug() << "next song index: " << lastSongIndex;

        currentTitle = ui->treeWidget->topLevelItem(lastSongIndex)->text(0);
        currentArtist = ui->treeWidget->topLevelItem(lastSongIndex)->text(1);
        currentUrl = ui->treeWidget->topLevelItem(lastSongIndex)->text(2);

        qDebug() << "next song url" << currentUrl;
        changeSong(currentUrl);

        play();
    }
    else{
        stop();
        lastSongIndex = -1;
    }
}

void amcpp::on_actionConfigure_triggered()
{
    configDialog c;
    c.show();

    if (c.exec() == 1)
        amHandshake();
}

void amcpp::checkStatus(Phonon::State act , Phonon::State prev){
    qDebug() << QString("%1 -> %2").arg(prev).arg(act) ;
}

