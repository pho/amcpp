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
        loadCollectionFromFile();
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

    ui->lineEdit->setEnabled(true);
}

void amcpp::on_playlistTree_itemDoubleClicked(QTreeWidgetItem *item, int /*column*/)
{
    stop();

    if(lastSongIndex >= 0){
        QFont font;
        font.setBold(false);
        ui->playlistTree->topLevelItem(lastSongIndex)->setFont(0, font);
        ui->playlistTree->topLevelItem(lastSongIndex)->setFont(1, font);
    }

    currentTitle = item->text(0);
    currentArtist = item->text(1);
    currentUrl = item->text(2);

    lastSongIndex = ui->playlistTree->indexOfTopLevelItem(item);
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

    ui->statusBar->clearMessage();
    ui->statusBar->showMessage(currentTitle + " - " + currentArtist);
    ui->titleLabel->setText(currentTitle + " - " + currentArtist);
    ui->totalLabel->setText(QString("%1").arg((float) mediaObject->totalTime()/60000)); //QTime

}

void amcpp::stop(){
    mediaObject->stop();
    ui->playButton->setText("Play");
}

void amcpp::nextSong(){
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

        changeSong(currentUrl);

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

void amcpp::checkStatus(Phonon::State act  , Phonon::State prev){
    //qDebug() << QString("States: %1 -> %2").arg(prev).arg(act);
    if (act == Phonon::StoppedState){
        if(lastSongIndex >= 0){
            QFont font;
            font.setBold(false);
            ui->playlistTree->topLevelItem(lastSongIndex)->setFont(0, font);
            ui->playlistTree->topLevelItem(lastSongIndex)->setFont(1, font);
        }
    }
    else if(act == Phonon::PlayingState){
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
                            QRegExp("ssid=.*&"), "ssid=" + authToken + "&");

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

        ui->statusBar->showMessage(QString("Scanning %1 songs").arg(c++));
    }
    ui->artistTree->sortItems(0, Qt::AscendingOrder);
    ui->statusBar->showMessage(QString("Collection loaded. %1 songs.").arg(c));

}

void amcpp::downloadProgress(qint64 received, qint64 total){
    ui->statusBar->showMessage(QString("Downloading collection... %1 / %2").arg(received/1024).arg(total));
}

void amcpp::on_loadCollectionButton_clicked()
{
    ui->loadCollectionButton->setDisabled(true);
    loadCollection();
    ui->loadCollectionButton->setDisabled(false);
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

    if(lastSongIndex == -1 && mediaObject->state() != Phonon::PlayingState){
        nextSong();
    }
}

QFile* amcpp::getCollectionFile(){

    //http://labs.qt.nokia.com/2008/02/26/finding-locations-with-qdesktopservices/
    QString directory = QDesktopServices::storageLocation(QDesktopServices::DataLocation);
    if (directory.isEmpty())
        directory = QDir::homePath() + "/." + QCoreApplication::applicationName();
    if (!QFile::exists(directory)) {
        QDir dir;
        dir.mkpath(directory);
    }
    QFile * file = new QFile(directory + "/collection.dat");
    return file;
}

void amcpp::saveCollection(QByteArray data){

    QFile * file = getCollectionFile();

    //
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

}
