#ifndef MPLAYERINTERLACEDETECTION_H
#define MPLAYERINTERLACEDETECTION_H

#include <QTime>
#include <QHash>
#include <QString>
#include <QProcess>
#include <QStringList>
#include <QMainWindow>
#include "ui_MPlayerInterlaceDetection.h"

class MPlayerInterlaceDetection : public QMainWindow
{
  Q_OBJECT

  public:
    MPlayerInterlaceDetection(QWidget *parent = 0);
    ~MPlayerInterlaceDetection();

  private:
    Ui::MPlayerInterlaceDetectionClass *m_ui;
    QProcess *m_process;
    bool m_running;
    QString m_currentInput;
    int m_run, m_firstLightAffinity01, m_firstLightAffinity12, m_firstLightAffinity23;
    int m_firstStrongAffinity01, m_firstStrongAffinity12, m_firstStrongAffinity23,
        m_firstTelecine32Count;
    int m_firstFullLight, m_firstFullStrong, m_currentIndex;
    QStringList m_files, m_data, m_firstAffinity, m_firstBreaks;
    QTime m_startTime;
    double m_percent;
    bool checkTools();
    void startAnalyse();
    void analyse();
    QString result(int lightAffinity01, int lightAffinity12, int lightAffinity23,
                   int strongAffinity01, int strongAffinity12, int strongAffinity23,
                   int telecine32Count, QStringList affinity, QStringList breaks, int count,
                   int fullLight, int fullStrong);
    void getFileLength();
    QHash<QString, double> m_length;
    QString buildAnalyseCall(const QString &input);

  private slots:
    void on_removePushButton_clicked();
    void on_analysePushButton_clicked();
    void handleMPlayerOutput();
    void handleMPlayerAnalyseOutput();
    void mplayerFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void mplayerAnalyseFinished(int exitCode, QProcess::ExitStatus exitStatus);
};

#endif // MPLAYERINTERLACEDETECTION_H
