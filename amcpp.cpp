#include "amcpp.h"
#include "ui_amcpp.h"
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
    QCA::Initializer init;

    authToken = "noauth";
    lastSongIndex = -1;

    audioOutput = new Phonon::AudioOutput(Phonon::MusicCategory, this);
    audioOutput->setVolume(0.5);

    mediaObject = new Phonon::MediaObject(this);
    mediaSource = new Phonon::MediaSource(QString("http://ampache/play/index.php?ssid=cd201228ee4a492ae74cd7db60822eda&oid=555&uid=2&name=/SoulEye%20-%20Path%20complete.mp3"));
    mediaObject->setCurrentSource(*mediaSource);

    Phonon::createPath(mediaObject, audioOutput);

    ui->setupUi(this);

    ui->seekSlider->setMediaObject(mediaObject);
    ui->volumeSlider->setAudioOutput(audioOutput);

    amHandshake();


    connect(mediaObject, SIGNAL(finished()), this, SLOT(nextSong()));

    audioOutput->setVolume(0.4);

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
        if (mediaObject->state() == Phonon::PausedState)
            play();
        else{
            if(ui->treeWidget->children().count() <= 0) return;
//            lastSongIndex = 0;
//            changeSong(ui->treeWidget->itemAt(0, 0)->text(2));
        }
    }
}


void amcpp::amHandshake(){
    if ( !QCA::isSupported("sha256") ){
        ui->statusBar->showMessage("sha256 is not supported. Exiting...\n");
        //burn da haus
        return;
    }
    else {
        ui->statusBar->showMessage("sha256 is supported :D\n");

        ui->statusBar->showMessage("Creating handshake\n");

        char key[80];
        char timestamp[11];
        snprintf(timestamp, 11, "%ld", time(0));

        QString lol(QCA::Hash("sha256").hash("test").toByteArray().toHex());

        snprintf(key, 80, "%s%s", timestamp, qPrintable(lol));

        QString passphrase = QCA::Hash("sha256").hashToString( key );

        QUrl url( QString("http://ampache/server/xml.server.php?action=handshake&auth=%1&timestamp=%2&version=350001&user=test").arg(qPrintable(passphrase), timestamp)) ;
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
    ui->lineEdit->setDisabled(true);
    QUrl url( QString("http://ampache/server/xml.server.php?action=search_songs&auth=%1&filter=%2").arg(authToken, ui->lineEdit->text())) ;
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

    ui->lineEdit->setEnabled(true);
}

void amcpp::on_treeWidget_itemDoubleClicked(QTreeWidgetItem *item, int column)
{
    stop();

    currentTitle = item->text(0);
    currentArtist = item->text(1);
    currentUrl = item->text(2);

    lastSongIndex = ui->treeWidget->indexOfTopLevelItem(item);
    changeSong(item->text(2));

    play();

}

void amcpp::play(){
    mediaObject->play();
    ui->playButton->setText("Pause");
}


void amcpp::pause(){
    mediaObject->pause();
    ui->playButton->setText("Play");
}

void amcpp::changeSong(QString str){

    delete mediaSource;

    mediaSource = new Phonon::MediaSource(str);
    mediaObject->setCurrentSource(*mediaSource);

   // ui->seekSlider->setMediaObject(mediaObject);

    ui->statusBar->clearMessage();
    ui->statusBar->showMessage(currentTitle + " - " + currentArtist);

    if(ui->seekSlider->isEnabled())
        ui->statusBar->showMessage("SeekEnabled");

}

void amcpp::stop(){
    mediaObject->stop();
    ui->playButton->setText("Play");
}

void amcpp::nextSong(){

    if (lastSongIndex == -1) return;

    if( ui->treeWidget->size().height() > ++lastSongIndex){

        currentTitle = ui->treeWidget->itemAt(lastSongIndex,2)->text(0);
        currentArtist = ui->treeWidget->itemAt(lastSongIndex,2)->text(1);
        currentUrl = ui->treeWidget->itemAt(lastSongIndex,2)->text(2);

        changeSong(currentUrl);

        play();
    }
    else{
        lastSongIndex = -1;
    }
}
