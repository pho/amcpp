#ifndef AMCPP_H
#define AMCPP_H

#include <QMainWindow>
#include <QCoreApplication>

//#include <phonon/audiooutput.h>
//#include <phonon/seekslider.h>
//#include <phonon/mediaobject.h>
//#include <phonon/volumeslider.h>
//#include <phonon/backendcapabilities.h>

#include <QtMultimedia/QMediaPlayer>
#include <QtMultimedia/QMediaPlaylist>

#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkAccessManager>
#include <QNetworkReply>

#include <QtXml/QDomDocument>
#include <QCryptographicHash>
#include <QTreeWidgetItem>
#include <QSettings>
#include <QFile>

namespace Ui {
    class amcpp;
}

class amcpp : public QMainWindow
{
    Q_OBJECT

public:
    explicit amcpp(QWidget *parent = 0);
    ~amcpp();

protected:
    void changeEvent(QEvent *e);

private slots:
    void play();
    void pause();
    void stop();
    void changeSong(QString);
    void nextSong();

    void checkStatus( QMediaPlayer::State,  QMediaPlayer::State);

    void loadCollection();
    void loadCollectionReply();
    void loadCollectionFromFile();
    void saveCollection(QByteArray);
    QFile* getCollectionFile();
    void downloadProgress(qint64, qint64);
    void on_playButton_clicked();
    void handshakeReply();
    void searchReply();
    void on_searchButton_clicked();

    void on_playlistTree_itemDoubleClicked(QTreeWidgetItem *item, int column);
    void on_actionConfigure_triggered();
    void on_loadCollectionButton_clicked();

    void on_searchTree_itemDoubleClicked(QTreeWidgetItem *item, int column);
    void on_artistTree_itemDoubleClicked(QTreeWidgetItem *item, int column);

    void on_nextButton_clicked();

    void on_clearButton_clicked();

private:
    Ui::amcpp *ui;
//    Phonon::AudioOutput *audioOutput;
//    Phonon::MediaObject *mediaObject;
//    Phonon::MediaSource *mediaSource;

    QMediaPlayer *mediaPlayer;
    QMediaPlaylist *playlist;
    QNetworkReply *reply;
    QNetworkAccessManager manager;
    QDomDocument domDocument;

    QString authToken;

    QString currentTitle, currentArtist, currentUrl;

    int lastSongIndex;


    void amHandshake();
    void addArtist(QTreeWidgetItem*);
    void addAlbum(QTreeWidgetItem*);
    void addSong(QTreeWidgetItem*);

};

#endif // AMCPP_H
