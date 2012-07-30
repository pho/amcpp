#ifndef AMCPP_H
#define AMCPP_H

#include <QMainWindow>
#include <QCoreApplication>

#include <phonon/audiooutput.h>
#include <phonon/seekslider.h>
#include <phonon/mediaobject.h>
#include <phonon/volumeslider.h>
#include <phonon/backendcapabilities.h>

#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkAccessManager>
#include <QNetworkReply>

#include <QtXml/QDomDocument>
#include <QtCrypto/QtCrypto>
#include <QTreeWidgetItem>
#include <QSettings>

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

    void checkStatus(Phonon::State, Phonon::State);

    void loadCollection();
    void loadCollectionReply();
    void downloadProgress(qint64, qint64);
    void on_playButton_clicked();
    void handshakeReply();
    void searchReply();
    void on_searchButton_clicked();

    void on_treeWidget_itemDoubleClicked(QTreeWidgetItem *item, int column);
    void on_actionConfigure_triggered();
    void on_loadCollectionButton_clicked();

private:
    Ui::amcpp *ui;
    Phonon::AudioOutput *audioOutput;
    Phonon::MediaObject *mediaObject;
    Phonon::MediaSource *mediaSource;
    QNetworkReply *reply;
    QNetworkAccessManager manager;
    QDomDocument domDocument;

    QString authToken;

    QString currentTitle, currentArtist, currentUrl;

    int lastSongIndex;


    void amHandshake();

};

#endif // AMCPP_H
