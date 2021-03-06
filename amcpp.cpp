#include "amcpp.h"
#include "ui_amcpp.h"
#include "configdialog.h"
//#include <phonon/mediaobject.h>

#include <QtMultimedia/QMediaPlayer>
#include <QtMultimedia/QMediaPlaylist>

#include <QGraphicsWebView>

#include <QList>
#include <QFileDialog>
#include <QDesktopServices>
#include <cstdlib>
#include <iostream>
#include <ctime>

amcpp::amcpp(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::amcpp)
{
    authToken = "noauth";
    lastSongIndex = -1;

    mediaPlayer = new QMediaPlayer;
    mediaPlayer->setVolume(50);
    playlist = new QMediaPlaylist;
    mediaPlayer->setPlaylist(playlist);

    ui->setupUi(this);

    //ui->seekSlider->setMediaObject(mediaObject);
    //ui->volumeSlider->setAudioOutput(audioOutput);



//    connect(mediaPlayer, SIGNAL(stateChanged(QMediaPlayer::State)),
//            this, SLOT(checkStatus(QMediaPlayer::State)));

    connect(playlist, SIGNAL(currentIndexChanged(int)), this, SLOT(changeSong(int)));

    connect(ui->volumeSlider, SIGNAL(sliderMoved(int)), mediaPlayer, SLOT(setVolume(int)));
    connect(mediaPlayer, SIGNAL(durationChanged(qint64)), this, SLOT(durationChanged(qint64)));
    connect(mediaPlayer, SIGNAL(positionChanged(qint64)), this, SLOT(positionChanged(qint64)));


    // No funciona bien el seek :/
    //connect(ui->seekSlider, SIGNAL(sliderReleased()), this, SLOT(setSeekPosition()));

    amHandshake();
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
    if(mediaPlayer->state() == QMediaPlayer::PlayingState){
        pause();
    }
    else{
        play();
    }
}

void amcpp::amHandshake(){

    QCryptographicHash sha256(QCryptographicHash::Sha256);
    authToken = "noauth";

    ui->statusBar->showMessage("Creating handshake\n");

    QSettings settings("amcpp", "amcpp");

    QString amurl = settings.value("amUrl").toString();
    QString user = settings.value("username").toString();


    char key[80];
    char timestamp[11];
    snprintf(timestamp, 11, "%ld", time(0));

    QString lol(settings.value("streamPass").toByteArray());

    snprintf(key, 80, "%s%s", timestamp, qPrintable(lol));

    QString passphrase = sha256.hash(key,QCryptographicHash::Sha256).toHex();

    QUrl url( QString("%3/server/xml.server.php?action=handshake&auth=%1&timestamp=%2&version=350001&user=%4").arg(qPrintable(passphrase), timestamp, amurl, user )) ;

    qDebug() << url;

    QNetworkRequest request(url);
    reply = manager.get(request);

    connect(reply, SIGNAL(readyRead()),
            this, SLOT(handshakeReply()));

}

void amcpp::durationChanged(qint64 d){
    ui->seekSlider->setMaximum(d/1000);
    int m;
    float s;
    s = d/1000;
    m = s/60;
    s = int( ((s/60) - m) * 60 );
    if(s < 10)
        ui->totalTime->setText(QString("%1:0%2").arg(m).arg(s));
    else
        ui->totalTime->setText(QString("%1:%2").arg(m).arg(s));
}

void amcpp::positionChanged(qint64 d){
    ui->seekSlider->setValue(d/1000);
    int m;
    float s;
    s = d/1000;
    m = s/60;
    s = int( ((s/60) - m) * 60 );
    if(s < 10)
        ui->currTime->setText(QString("%1:0%2").arg(m).arg(s));
    else
        ui->currTime->setText(QString("%1:%2").arg(m).arg(s));
}

void amcpp::setSeekPosition(){
    if(mediaPlayer->isSeekable())
        mediaPlayer->setPosition(ui->seekSlider->value());
    else
        qDebug() << "Media not seekable";
}

void amcpp::handshakeReply()
{

    QDomDocument e;
    e.setContent(reply->readAll());

    qDebug() << "Got reply";

    if (e.firstChildElement().firstChildElement().tagName() == "auth"){
        authToken = e.firstChildElement().firstChildElement().text();

        ui->statusBar->showMessage("Auth completed!");
        qDebug() << "Auth Completed!";
        loadCollectionFromFile();
    }
    else{
        ui->statusBar->showMessage("Auth invalid!!");
        qDebug() << "Auth Failed!";
        //TODO Disable all
    }
}


void amcpp::on_searchButton_clicked()
{
    QSettings settings("amcpp", "amcpp");
    QString amurl = settings.value("amUrl").toString();

    ui->searchEdit->setDisabled(true);
    QUrl url( QString("%3/server/xml.server.php?action=search_songs&auth=%1&filter=%2").arg(authToken, ui->searchEdit->text(), amurl)) ;
    QNetworkRequest request(url);
    reply = manager.get(request);

    connect(reply, SIGNAL(finished()),
            this, SLOT(searchReply()));
}

void amcpp::searchReply()
{
    QDomDocument e;
    e.setContent(reply->readAll());

    ui->searchTree->clear();

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
        ui->searchTree->addTopLevelItem(item);
    }

    ui->searchEdit->setEnabled(true);
}


void amcpp::on_playlistTree_itemDoubleClicked(QTreeWidgetItem *item, int /*column*/)
{
    playlist->setCurrentIndex(ui->playlistTree->indexOfTopLevelItem(item));
    play();
    return;
}

void amcpp::play(){
    if (lastSongIndex == -1)
        nextSong();
    else {
        mediaPlayer->play();
        ui->playButton->setText("Pause");
    }
}


void amcpp::pause(){
    mediaPlayer->pause();
    ui->playButton->setText("Play");
}

void amcpp::changeSong(int index){

    qDebug() << "Change song: " << index;

    QTreeWidgetItem* item = ui->playlistTree->topLevelItem(index);

    QFont font;
    if(lastSongIndex >= 0){
        font.setBold(false);
        ui->playlistTree->topLevelItem(lastSongIndex)->setFont(0, font);
        ui->playlistTree->topLevelItem(lastSongIndex)->setFont(1, font);
    }

    font.setBold(true);
    ui->playlistTree->topLevelItem(index)->setFont(0, font);
    ui->playlistTree->topLevelItem(index)->setFont(1, font);

    lastSongIndex = index;


    currentTitle = item->text(0);
    currentArtist = item->text(1);
    currentUrl = item->text(2);

    qDebug() << item->text(0);

    ui->statusBar->clearMessage();
    ui->statusBar->showMessage(currentTitle + " - " + currentArtist);
    ui->titleLabel->setText(currentTitle + " - " + currentArtist);

//    QGraphicsWebView art = QGraphicsWebView;
//    art.load();
//    ui->graphicsView->setart.graphicsItem();

    this->setWindowTitle(currentTitle + " - " + currentArtist);

}

void amcpp::stop(){
    mediaPlayer->stop();
    ui->playButton->setText("Play");
}

void amcpp::prevSong(){
    mediaPlayer->playlist()->previous();
    mediaPlayer->play();
    return;
}

void amcpp::nextSong(){

    mediaPlayer->playlist()->next();
    play();
    return;


    stop();

    if(lastSongIndex >= 0){
        QFont font;
        font.setBold(false);
        ui->playlistTree->topLevelItem(lastSongIndex)->setFont(0, font);
        ui->playlistTree->topLevelItem(lastSongIndex)->setFont(1, font);
    }


    if( ui->playlistTree->topLevelItemCount() > ++lastSongIndex){

        currentTitle = ui->playlistTree->topLevelItem(lastSongIndex)->text(0);
        currentArtist = ui->playlistTree->topLevelItem(lastSongIndex)->text(1);
        currentUrl = ui->playlistTree->topLevelItem(lastSongIndex)->text(2);

        changeSong(lastSongIndex);

        play();
    }
    else{
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

void amcpp::checkStatus(QMediaPlayer::State act){
    qDebug() << QString("State: %1").arg(act);
    if (act ==  QMediaPlayer::StoppedState){
        if(lastSongIndex >= 0){
            QFont font;
            font.setBold(false);
            ui->playlistTree->topLevelItem(lastSongIndex)->setFont(0, font);
            ui->playlistTree->topLevelItem(lastSongIndex)->setFont(1, font);
        }
    }
    else if(act == QMediaPlayer::PlayingState){
        if(lastSongIndex >= 0){
            QFont font;
            font.setBold(true);
            ui->playlistTree->topLevelItem(lastSongIndex)->setFont(0, font);
            ui->playlistTree->topLevelItem(lastSongIndex)->setFont(1, font);
        }
    }
}

void amcpp::loadCollection(){

    ui->statusBar->showMessage("Loading full collection...");

    ui->tabWidget->setEnabled(false);
    ui->artistTree->setEnabled(false);
    ui->searchTree->setEnabled(false);


    QSettings settings("amcpp", "amcpp");
    QString amurl = settings.value("amUrl").toString();

    QUrl url( QString("%2/server/xml.server.php?action=songs&auth=%1").arg(authToken, amurl)) ;
    QNetworkRequest request(url);
    reply = manager.get(request);

    connect(reply, SIGNAL(finished()),
            this, SLOT(loadCollectionReply()));
    connect(reply, SIGNAL(downloadProgress(qint64,qint64)), this, SLOT(downloadProgress(qint64, qint64)));
}

void amcpp::loadCollectionReply(){
    saveCollection(reply->readAll());
    loadCollectionFromFile();
}

void amcpp::loadCollectionFromFile(){
    ui->artistTree->clear();

    QDomDocument e;
    QFile* file = getCollectionFile();
    file->open(QIODevice::ReadOnly);

    if(file->size() == 0){
        qDebug() << "collection.dat is empty";
        loadCollection();
        return;
    }

    e.setContent(file->readAll());

    //TODO check if DomDocument is correct

    int c = 0;

    QDomElement el = e.firstChildElement();

    for( int i = 0; i < el.childNodes().count(); i++ ){
        QDomElement song = el.childNodes().at(i).toElement();
        QString artist, album, title, url;

        for ( int j = 0; j < song.childNodes().count(); j++){
            QString tag = song.childNodes().at(j).toElement().tagName();

            if (tag =="title")
                title = song.childNodes().at(j).toElement().text();

            if (tag == "artist")
                artist = song.childNodes().at(j).toElement().text();

            if (tag == "url")
                url = song.childNodes().at(j).toElement().text().replace(
                            //TODO: This regexp should match the ssid without reference to oid
                            QRegExp("ssid=.*&oid"), "ssid=" + authToken + "&oid");

            if (tag == "album")
                album = song.childNodes().at(j).toElement().text();

        }

        QTreeWidgetItem *item = new QTreeWidgetItem(1002); //type 1002 == song
        item->setText(0, title);
        item->setText(1, album);
        item->setText(2, url);

        QTreeWidgetItem *Artist, *Album;
        QList<QTreeWidgetItem *> rArtist = ui->artistTree->findItems(artist, Qt::MatchExactly);

        if(rArtist.count() < 1){
            Artist = new QTreeWidgetItem(QStringList(artist), 1000); //type 1000 == artist
            ui->artistTree->addTopLevelItem(Artist);
        }
        else{
            Artist = rArtist.at(0);
        }

//        QList<QTreeWidgetItem *> rAlbum = Artist->treeWidget()->children() album);

//        if(rAlbum.count() < 1){
//            qDebug() << "Artist" << artist << "." << album << "album not found";
//            Album = new QTreeWidgetItem(QStringList(album));
//            Artist->addChild(Album);
//        }
//        else{
//            qDebug() << "Artist" << artist << "." << album << "album";
//            Album = rAlbum.at(0);
//        }

        bool found = false;
        for( int i = 0; i < Artist->childCount(); i++){
            if(Artist->child(i)->text(0) == album){
                Artist->child(i)->addChild(item);
                found = true;
            }
        }
        if (!found){
            Album = new QTreeWidgetItem(QStringList(album), 1001); //type 1001 == album
            Album->addChild(item);
            Artist->addChild(Album);
        }


        if(++c%100 == 0)
            ui->statusBar->showMessage(QString("Scanning %1 songs").arg(c));
    }
    ui->artistTree->sortItems(0, Qt::AscendingOrder);
    ui->statusBar->showMessage(QString("Collection loaded. %1 songs.").arg(c));
    ui->tabWidget->setEnabled(true);
    ui->artistTree->setEnabled(true);
    ui->searchTree->setEnabled(true);
}

void amcpp::downloadProgress(qint64 received, qint64 total){
    ui->statusBar->showMessage(QString("Downloading collection... %1 / %2").arg(received/1024).arg(total));
}


void amcpp::on_searchTree_itemDoubleClicked(QTreeWidgetItem *item, int /*column*/)
{
    ui->statusBar->showMessage("Added to playlist");
    addSong(item);
}

void amcpp::on_artistTree_itemDoubleClicked(QTreeWidgetItem *item, int /*column*/)
{
    switch(item->type()){
        case 1000:
            addArtist(item);
            break;
        case 1001:
            addAlbum(item);
            break;
        case 1002:
            addSong(item);
            break;
        default:
            qDebug() << "[artistTree_itemDoubleClicked] I dont know what is this.";
    }
}

void amcpp::addArtist(QTreeWidgetItem* item){
    if (item->type() != 1000){
        qDebug() << "This is not an artist item";
        return;
    }
    for(int i = 0; i < item->childCount(); i++)
        addAlbum(item->child(i));
}

void amcpp::addAlbum(QTreeWidgetItem* item){
    if (item->type() != 1001){
        qDebug() << "This is not an album item";
        return;
    }
    for(int i = 0; i < item->childCount(); i++)
        addSong(item->child(i));
}

void amcpp::addSong(QTreeWidgetItem* item){
    QTreeWidgetItem * newitem = new QTreeWidgetItem(
                QStringList() << item->text(0) << item->text(1) << item->text(2));
    ui->playlistTree->addTopLevelItem(newitem);

    playlist->addMedia(QMediaContent(QUrl(item->text(2))));

    if(lastSongIndex == -1 && mediaPlayer->state() !=  QMediaPlayer::PlayingState){
        nextSong();
    }
}

QFile* amcpp::getCollectionFile(){

    //http://labs.qt.nokia.com/2008/02/26/finding-locations-with-qdesktopservices/
    QString directory = QStandardPaths::writableLocation(QStandardPaths::DataLocation);
    qDebug() << directory;
    if (directory.isEmpty())
        directory = QDir::homePath() + "/." + QCoreApplication::applicationName();
    if (!QFile::exists(directory)) {
        QDir dir;
        dir.mkpath(directory);
    }
    QFile * file = new QFile(directory + "/collection.dat");
    qDebug() << "Getting collection file";
    return file;
}

void amcpp::saveCollection(QByteArray data){

    QFile * file = getCollectionFile();


    //From Network Download Example
    if (!file->open(QIODevice::WriteOnly)) {
        fprintf(stderr, "Could not open %s for writing: %s\n",
                qPrintable(file->fileName()),
                qPrintable(file->errorString()));
        return;
    }
    //

    file->write(data);
    file->close();

    delete file;

}

void amcpp::on_nextButton_clicked()
{
    nextSong();
}

void amcpp::on_clearButton_clicked()
{
    lastSongIndex = -1;
    ui->playlistTree->clear();
    mediaPlayer->playlist()->clear();

}

void amcpp::on_prevButton_clicked()
{
    prevSong();
}

void amcpp::on_stopButton_clicked()
{
    stop();
}
