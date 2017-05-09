#include "MPlayerInterlaceDetection.h"
#include <QFile>
#include <QList>
#include <QThread>
#include <QTime>
#include <QMessagebox>
#include <QListWidgetItem>
#include <windows.h>

MPlayerInterlaceDetection::MPlayerInterlaceDetection(QWidget *parent) :
    QMainWindow(parent), m_ui(0), m_process(0), m_running(false), m_currentInput(QString()),
        m_run(0), m_firstLightAffinity01(0), m_firstLightAffinity12(0), m_firstLightAffinity23(0),
        m_firstStrongAffinity01(0), m_firstStrongAffinity12(0), m_firstStrongAffinity23(0),
        m_firstTelecine32Count(0), m_firstFullLight(0), m_firstFullStrong(0), m_currentIndex(0),
        m_files(), m_data(), m_firstAffinity(), m_firstBreaks(), m_startTime(), m_percent(100),
        m_length()
{
  m_ui = new Ui::MPlayerInterlaceDetectionClass();
  m_ui->setupUi(this);
  this->setWindowTitle("MPlayerInterlaceDetection");
}

MPlayerInterlaceDetection::~MPlayerInterlaceDetection()
{
  if (m_process != 0) {
    m_process->disconnect();
    m_process->kill();
    delete m_process;
  }
}

void MPlayerInterlaceDetection::on_removePushButton_clicked()
{
  QList<QListWidgetItem *> items = m_ui->listWidget->selectedItems();
  foreach(QListWidgetItem *item, items) {
    m_ui->listWidget->remove(item);
  }
}

bool MPlayerInterlaceDetection::checkTools()
{
  QString lookingFor = "mplayer2";
#ifdef Q_OS_WIN32
  lookingFor += ".exe";
#endif
  bool mplayer = QFile::exists(lookingFor);
  QString text = tr("mplayer exists: %1").arg(QString(mplayer ? "true" : "false"));

  if (mplayer) {
    return true;
  }
  text += "\r\n";
  text += tr("MPlayer2 is needed!");
  QMessageBox::information(this, tr("Error"), text);
  return false;
}

void MPlayerInterlaceDetection::on_analysePushButton_clicked()
{
  if (!this->checkTools() || m_running) {
    return;
  }
  m_currentIndex = 0;
  m_ui->textBrowser->clear();
  m_percent = m_ui->toAnalyseDoubleSpinBox->value();
  m_ui->textBrowser->append(tr("Going to analyze %1% of each source,..").arg(m_percent));
  m_files = QStringList();
  int count = m_ui->listWidget->count();
  if (count == 0) {
    return;
  }
  m_run = 0;
  m_currentInput = QString();
  QString item;
  for (int i = 0; i < count; ++i) {
    item = m_ui->listWidget->item(i)->text();
    m_files << item;
  }
  if (m_percent > 0 && m_percent < 100) {
    this->getFileLength();
  } else {
    this->startAnalyse();
  }
}
bool tsMuxeRCompatible(const QString input)
{
  return input.endsWith(".m2ts", Qt::CaseInsensitive) || input.endsWith(".mts", Qt::CaseInsensitive)
      || input.endsWith(".ts", Qt::CaseInsensitive) || input.endsWith(".mpls", Qt::CaseInsensitive);
}

bool movCompatible(const QString input)
{
  return input.endsWith("mov", Qt::CaseInsensitive);
}

bool matroskaCompatible(const QString input)
{
  return input.endsWith("mkv", Qt::CaseInsensitive) || input.endsWith("mka", Qt::CaseInsensitive)
      || input.endsWith("webm", Qt::CaseInsensitive);
}

bool sfdCompatible(const QString input)
{
  return input.endsWith("sfd", Qt::CaseInsensitive);
}

void shortName(QString &inputFile)
{
  wchar_t* input = new wchar_t[inputFile.size() + 1];
  inputFile.toWCharArray(input);
  input[inputFile.size()] = L'\0';
  long length = GetShortPathName(input, NULL, 0);
  wchar_t* output = new wchar_t[length];
  GetShortPathName(input, output, length);
  inputFile = QString::fromWCharArray(output, length - 1);
  delete[] input;
  delete[] output;
}

QString MPlayerInterlaceDetection::buildAnalyseCall(const QString &input)
{
  QString inputfile = input.trimmed();
  QStringList analyseCall;
  analyseCall << "mplayer2";
  analyseCall << "-frames 0";
  analyseCall << "-v";
  analyseCall << "-identify";
  if (sfdCompatible(inputfile) || tsMuxeRCompatible(inputfile)) {
    analyseCall << "-demuxer 35";
  } else if (matroskaCompatible(inputfile)) {
    analyseCall << "-demuxer mkv";
    analyseCall << "-nocorrect-pts";
  } else if (movCompatible(inputfile)) {
    analyseCall << "-demuxer mov";
  }

  analyseCall << "-vo null -ao null";
  shortName(inputfile);
  analyseCall << "\"" + inputfile + "\"";
  return analyseCall.join(" ");
}

void MPlayerInterlaceDetection::getFileLength()
{
  delete m_process;
  m_process = new QProcess(this);
  QObject::connect(m_process, SIGNAL(readyReadStandardOutput()), this,
                   SLOT(handleMPlayerAnalyseOutput()));
  QObject::connect(m_process, SIGNAL(finished(int, QProcess::ExitStatus)), this,
                   SLOT(mplayerAnalyseFinished(int, QProcess::ExitStatus)));

  QString call = buildAnalyseCall(m_files.at(m_currentIndex));
  m_ui->textBrowser->append(tr("Grabbing length from: %1").arg(call));
  m_process->start(call);
}

void MPlayerInterlaceDetection::startAnalyse()
{
  if (m_run == 0) {
    m_currentInput = m_files.takeFirst();
  }
  m_run++;
  m_ui->textBrowser->append("startAnalyse (" + QString::number(m_run) + ") for " + m_currentInput);
  delete m_process;
  m_process = new QProcess(this);
  QObject::connect(m_process, SIGNAL(readyReadStandardOutput()), this, SLOT(handleMPlayerOutput()));
  QObject::connect(m_process, SIGNAL(finished(int, QProcess::ExitStatus)), this,
                   SLOT(mplayerFinished(int, QProcess::ExitStatus)));
  QStringList call;
#ifdef Q_OS_WIN32
  call << "mplayer2.exe";
#else
  call << "mplayer2";
#endif
  call << "-noframedrop";
  call << "-lavdopts threads=" + QString::number(QThread::idealThreadCount());
  call << "-speed 100";
  call << "-v";
  QString input = m_currentInput;
  shortName(input);
  call << "\"" + input + "\"";
  if (sfdCompatible(m_currentInput) || tsMuxeRCompatible(m_currentInput)) {
    call << "-demuxer 35";
  } else if (matroskaCompatible(m_currentInput)) {
    call << "-demuxer mkv";
    call << "-nocorrect-pts";
  } else if (movCompatible(m_currentInput)) {
    call << "-demuxer mov";
  }

  if (m_percent > 0 && m_percent < 100) {
    double length = m_length.value(m_currentInput);
    if (length > 100) {
      length *= m_percent / 100.0;
      int endPos = int(length);
      if (endPos > 100) {
        call << "-endpos " + QString::number(endPos);
      }
    }
  }

  call << "-nosound";
  call << "-vo null";
  call << "-ao null";
  call << "-nosub";
  switch (m_run) {
    case 1 :
      call << "-vf pullup";
      break;
    case 2 :
      call << "-vf lb,pullup";
      break;
    default :
      break;
  }

  QString executionCall = call.join(" ");
  m_ui->textBrowser->append(executionCall);
  m_data.clear();
  m_running = true;
  m_startTime.start();
  m_process->start(executionCall);
}

void MPlayerInterlaceDetection::handleMPlayerAnalyseOutput()
{
  QString text = m_process->readAllStandardOutput().data();
  if (text.isEmpty()) {
    return;
  }
  QString line;
  QStringList lines = text.split("\n");
  for (int i = 0, c = lines.count(); i < c; ++i) {
    line = lines.at(i).trimmed();
    if (line.startsWith("ID_LENGTH=")) {
      line = line.remove("ID_LENGTH=").trimmed();
      m_ui->textBrowser->append(" -> " + tr("length: %1s").arg(line.toDouble()));
      m_length.insert(m_files.at(m_currentIndex), line.toDouble());
      break;
    }
  }
}

void MPlayerInterlaceDetection::handleMPlayerOutput()
{
  QString text = m_process->readAllStandardOutput().data();
  if (text.isEmpty()) {
    return;
  }
  QString line;
  QStringList lines = text.split("\n");
  for (int i = 0, c = lines.count(); i < c; ++i) {
    line = lines.at(i).trimmed();
    if (line.startsWith("affinity") || line.startsWith("breaks") || line.startsWith("duration")) {
      m_data << line;
    } else if (line.startsWith("V:")) {
      m_data << line;

      line = line.remove(0, 3);
      line = line.trimmed();
      line = line.remove(line.indexOf(" "), line.size());
      m_ui->label->setText(line);
    }
  }
}

QString msToHMS(int el)
{

  int h = el / 3600000 > 0 ? el / 3600000 : 0;
  int m = el - h * 3600000 > 0 ? (el - h * 3600000) / 60000 : 0;
  int s = el - h * 3600000 - m * 60000 > 0 ? (el - (h * 3600000 + m * 60000)) / 1000 : 0;
  int ms =
      el - (h * 3600000 + m * 60000 + s * 1000) > 0 ? el - (h * 3600000 + m * 60000 + s * 1000) :
          el;
  QString hours = QString::number(h);
  if (h < 10) {
    hours = "0" + hours;
  }
  QString minutes = QString::number(m);
  if (m < 10) {
    minutes = "0" + minutes;
  }
  QString seconds = QString::number(1);
  if (s < 10) {
    seconds = "0" + seconds;
  }
  return QString("%1:%2:%3.%4").arg(hours).arg(minutes).arg(seconds).arg(ms);
}

void MPlayerInterlaceDetection::mplayerAnalyseFinished(int exitCode,
                                                       QProcess::ExitStatus exitStatus)
{
  if (exitCode < 0) {
    QMessageBox::critical(
        this, tr("Crashed,.."),
        tr("MPlayer crashed with error code: %1 -> %2").arg(exitCode).arg(exitStatus));
    return;
  }
  if (m_currentIndex + 1 < m_files.count()) {
    m_currentIndex++;
    this->getFileLength();
    return;
  }
  this->startAnalyse();
}

void MPlayerInterlaceDetection::mplayerFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
  QString ms = msToHMS(m_startTime.elapsed());
  m_ui->textBrowser->append(tr("mplayer finished (%1) after %2hrs").arg(m_run).arg(ms));
  m_running = false;
  if (exitCode < 0) {
    QMessageBox::critical(
        this, tr("Crashed,.."),
        tr("MPlayer crashed with error code: %1 -> %2").arg(exitCode).arg(exitStatus));
    return;
  }
  this->analyse();
}

QString MPlayerInterlaceDetection::result(int lightAffinity01, int lightAffinity12,
                                          int lightAffinity23, int strongAffinity01,
                                          int strongAffinity12, int strongAffinity23,
                                          int telecine32Count, QStringList affinity,
                                          QStringList breaks, int count, int fullLight,
                                          int fullStrong)
{
  const int firstLightAffinityCount = m_firstLightAffinity01 + m_firstLightAffinity12
      + m_firstLightAffinity23;
  double firstLightAffinityCountPercent =
      (count > 0) ? (100.0 * firstLightAffinityCount / count) : 0;
  const int lightAffinityCount = lightAffinity01 + lightAffinity12 + lightAffinity23;
  const int lightAffinityCountPercent = (count > 0) ? (100.0 * lightAffinityCount / count) : 0;
  int firstStrongAffinityCount = m_firstStrongAffinity01 + m_firstStrongAffinity12;
  firstStrongAffinityCount += m_firstStrongAffinity23;
  const double firstStrongAffinityCountPercent =
      (count > 0) ? (100.0 * firstStrongAffinityCount / count) : 0;
  const int strongAffinityCount = strongAffinity01 + strongAffinity12 + strongAffinity23;
  const int strongAffinityCountPercent = (count > 0) ? (100.0 * strongAffinityCount / count) : 0;
  const double firstTelecinePercent = (count > 0) ? (m_firstTelecine32Count * 100.0 / count) : 0;
  const double telecinePercent = (count > 0) ? (telecine32Count * 100.0 / count) : 0;
  QString infos = "\r\n  ";
  const int breakCount = breaks.count();
  const int firstBreakCount = m_firstBreaks.count();
  const int affinityCount = affinity.count();
  const int m_firstAffinityCount = m_firstAffinity.count();
  int affinityDiffer = 0, breakDiffer = 0;
  double affDiffPercent = 0, breakDiffPercent = 0;
  const int light01Percent = (count > 0) ? int(100.0 * lightAffinity01 / count) : 0;
  const int light12Percent = (count > 0) ? int(100.0 * lightAffinity12 / count) : 0;
  const int light23Percent = (count > 0) ? int(100.0 * lightAffinity23 / count) : 0;

  infos += tr("light affinity (0+) 0->1 : %1 of %2").arg(lightAffinity01).arg(count);
  infos += " (" + QString::number(light01Percent) + "%)";
  infos += "\r\n  ";
  infos += tr("light affinity (1+) 1->2 : %1 of %2").arg(lightAffinity12).arg(count);
  infos += " (" + QString::number(light12Percent) + "%)";
  infos += "\r\n  ";
  infos += tr("light affinity (2+) 2->3 : %1 of %2").arg(lightAffinity23).arg(count);
  infos += " (" + QString::number(light23Percent) + "%)";
  infos += "\r\n  ";
  const int firstLight01Percent = (count > 0) ? int(100.0 * m_firstLightAffinity01 / count) : 0;
  const int firstLight12Percent = (count > 0) ? int(100.0 * m_firstLightAffinity12 / count) : 0;
  const int firstLight23Percent = (count > 0) ? int(100.0 * m_firstLightAffinity23 / count) : 0;
  infos += tr("first light affinity (0+) 0->1 : %1 of %2").arg(m_firstLightAffinity01).arg(count);
  infos += " (" + QString::number(firstLight01Percent) + "%)";
  infos += "\r\n  ";
  infos += tr("first light affinity (1+) 1->2 : %1 of %2").arg(m_firstLightAffinity12).arg(count);
  infos += " (" + QString::number(firstLight12Percent) + "%)";
  infos += "\r\n  ";
  infos += tr("first light affinity (2+) 2->3 : %1 of %2").arg(m_firstLightAffinity23).arg(count);
  infos += " (" + QString::number(firstLight23Percent) + "%)";
  infos += "\r\n  ";
  infos += tr("first light affinity count: %1").arg(firstLightAffinityCount);
  infos += " (" + QString::number(int(firstLightAffinityCountPercent)) + "%)";
  infos += "\r\n  ";
  infos += tr("second light affinity count: %1").arg(lightAffinityCount);
  infos += " (" + QString::number(int(lightAffinityCountPercent)) + "%)";
  infos += "\r\n";
  infos += "\r\n  ";
  const int strong01Percent = (count > 0) ? int(100.0 * strongAffinity01 / count) : 0;
  const int strong12Percent = (count > 0) ? int(100.0 * strongAffinity12 / count) : 0;
  const int strong23Percent = (count > 0) ? int(100.0 * strongAffinity23 / count) : 0;
  infos += tr("strong affinity (0+) 0->1 : %1 of %2").arg(strongAffinity01).arg(count);
  infos += " (" + QString::number(strong01Percent) + "%)";
  infos += "\r\n  ";
  infos += tr("strong affinity (1+) 1->2 : %1 of %2").arg(strongAffinity12).arg(count);
  infos += " (" + QString::number(strong12Percent) + "%)";
  infos += "\r\n  ";
  infos += tr("strong affinity (2+) 2->3 : %1 of %2").arg(strongAffinity23).arg(count);
  infos += " (" + QString::number(strong23Percent) + "%)";
  infos += "\r\n  ";
  const int firstStrong01Percent = (count > 0) ? int(100.0 * m_firstStrongAffinity01 / count) : 0;
  const int firstStrong12Percent = (count > 0) ? int(100.0 * m_firstStrongAffinity12 / count) : 0;
  const int firstStrong23Percent = (count > 0) ? int(100.0 * m_firstStrongAffinity23 / count) : 0;
  infos += tr("first strong affinity (0+) 0->1 : %1 of %2").arg(m_firstStrongAffinity01).arg(count);
  infos += " (" + QString::number(firstStrong01Percent) + "%)";
  infos += "\r\n  ";
  infos += tr("first strong affinity (1+) 1->2 : %1 of %2").arg(m_firstStrongAffinity12).arg(count);
  infos += " (" + QString::number(firstStrong12Percent) + "%)";
  infos += "\r\n  ";
  infos += tr("first strong affinity (2+) 2->3 : %1 of %2").arg(m_firstStrongAffinity23).arg(count);
  infos += " (" + QString::number(firstStrong23Percent) + "%)";
  infos += "\r\n  ";
  infos += tr("first strong affinity count: %1").arg(firstStrongAffinityCount);
  infos += " (" + QString::number(int(firstStrongAffinityCountPercent)) + "%)";
  infos += "\r\n  ";
  infos += tr("second strong affinity count: %1").arg(strongAffinityCount);
  infos += " (" + QString::number(int(strongAffinityCountPercent)) + "%)";
  infos += "\r\n";
  infos += "\r\n  ";
  infos += tr("telecine 3:2 percentage (first): %1").arg(int(m_firstTelecine32Count));
  infos += "(" + QString::number(firstTelecinePercent) + ")";
  infos += "\r\n  ";
  infos += tr("telecine 3:2 percentage (second): %1").arg(int(telecine32Count));
  infos += "(" + QString::number(int(telecinePercent)) + ")";
  infos += "\r\n  ";
  infos += tr("Full light first: %1").arg(m_firstFullLight);
  infos += "\r\n  ";
  infos += tr("Full light second: %1").arg(fullLight);
  infos += "\r\n  ";
  infos += tr("Full strong first: %1").arg(m_firstFullStrong);
  infos += "\r\n  ";
  infos += tr("Full strong second: %1").arg(fullStrong);
  infos += "\r\n";
  infos += "\r\n  ";
  infos += tr("m_firstAffinity.count(): %1").arg(m_firstAffinityCount);
  infos += "\r\n  ";
  infos += tr("secondAffinity.count(): %1").arg(affinityCount);
  infos += "\r\n";
  if (affinityCount == m_firstAffinityCount) {
    for (int i = 0, c = affinityCount; i < c; ++i) {
      if (affinity.at(i) != m_firstAffinity.at(i)) {
        affinityDiffer++;
      }
    }
    affDiffPercent = (count > 0) ? (affinityDiffer * 100.0 / count) : 0;
    infos += "  " + tr("affinityDiffer: %1").arg(affinityDiffer);
    infos += "(" + QString::number(int(affDiffPercent)) + "%)";
    infos += "\r\n";
  }
  infos += "\r\n  ";
  infos += tr("m_firstBreaks.count(): %1").arg(firstBreakCount);
  infos += "\r\n  ";
  infos += tr("secondBreaks.count(): %1").arg(breakCount);
  infos += "\r\n";
  if (breakCount == firstBreakCount) {
    for (int i = 0, c = breakCount; i < c; ++i) {
      if (breaks.at(i) != m_firstBreaks.at(i)) {
        breakDiffer++;
      }
    }
    breakDiffPercent = (count > 0) ? (breakDiffer * 100.0 / count) : 0;
    infos += "  " + tr("breakDiffer: %1").arg(breakDiffer);
    infos += "(" + QString::number(int(breakDiffPercent)) + "%)";
    infos += "\r\n";
  }
  infos += "\r\n";

  //DECIDE WHAT IS WHAT
  if (firstTelecinePercent > 29 && telecinePercent < 2) {
    infos += tr("(firstTelecinePercent: %1").arg(firstTelecinePercent);
    infos += " " + tr("vs.") + " ";
    infos += tr("telecinePercent: %1)").arg(telecinePercent);
    infos += " => ";
    infos += "telecine  3:2";
    return infos;
  }
  int breakDiff = qAbs(breakCount - firstBreakCount);
  if ((breakDiff * 100.0) / (count * 1.0) > 0.5
      && (qAbs(affinityCount - m_firstAffinityCount) * 100.0) / (count * 1.0) > 0.5) {
    if ((breakDiff <= firstBreakCount / 500)
        && (qAbs(affinityCount - m_firstAffinityCount) <= m_firstAffinityCount / 500)) {
      infos += " => ";
      return infos + "progressive (0)";
    }

    if (int(telecinePercent) == 0) {
      if (firstTelecinePercent > 10) {
        infos += " => ";
        return infos + "telecine  (1)";
      }
      if (firstTelecinePercent > 5) {
        infos += " => ";
        bool fieldBlended = firstStrong01Percent >= 15 && m_firstStrongAffinity01 >= 15
            && strongAffinityCountPercent >= 15 && firstStrongAffinityCountPercent >= 15
            && firstLightAffinityCountPercent >= 15 && lightAffinityCountPercent >= 15;
        if (fieldBlended) {
          return infos + "field-blended";
        }
        return infos + "telecine (2)";
      }
    }
    double diffBreak = breakDiff * 100.0 / qMin(breakCount, firstBreakCount);
    double diffAff = qAbs(affinityCount - m_firstAffinityCount) * 100.0
        / qMin(affinityCount, m_firstAffinityCount);
    if (int(diffBreak) > 1 && int(diffAff) > 1 && affinityCount > m_firstAffinityCount
        && breakCount > firstBreakCount) {
      infos += " => ";
      return infos + tr("interlaced") + " (2)";
    }
    bool noTele = int(firstTelecinePercent) == 0 && int(telecinePercent) == 0;
    if (noTele) {
      infos += " => ";
      return infos + tr("interlaced") + " (3)";
    }
    if (firstTelecinePercent < 2 && int(telecinePercent) == 0) {
      infos += " => ";
      return infos + tr("interlaced") + " (4)";
    }
    infos += " => ";
    return infos + tr("mixed interlaced/progressive");
  }
  if (int(telecinePercent) == 0 && firstTelecinePercent > 10) {
    infos += " => ";
    return infos + "telecine (3)";
  }

  int lightCountPercentDiff = firstLightAffinityCountPercent - lightAffinityCountPercent;
  int strongCountPercentDiff = firstStrongAffinityCountPercent - strongAffinityCountPercent;
  if (affinityDiffer != 0 && qAbs(lightCountPercentDiff) < 13
      && qAbs(strongCountPercentDiff) < 13) {
    infos += " => ";
    return infos + "telecine (1)";
  }

  if (lightCountPercentDiff >= 8) {
    infos += " => ";
    return infos + tr("interlaced") + " (5)";
  }

  if (strongCountPercentDiff >= 8) {
    infos += " => ";
    return infos + tr("interlaced") + " (6)";
  }
  if (affDiffPercent >= 23) {
    infos += " => ";
    return infos + tr("interlaced") + " (7)";
  }
  if (breakDiffPercent >= 18) {
    infos += " => ";
    return infos + tr("interlaced") + " (8)";
  }
  if ((firstLight01Percent > 18 && light01Percent < 2)
      || (firstLight12Percent > 18 && light12Percent < 2)
      || (firstLight23Percent > 18 && light23Percent < 2)) {
    infos += " => ";
    return infos + tr("interlaced") + " (9)";
  }
  if ((firstStrong01Percent > 18 && strong01Percent < 2)
      || (firstStrong12Percent > 18 && strong12Percent < 2)
      || (firstStrong23Percent > 18 && strong23Percent < 2)) {
    infos += " => ";
    return infos + tr("interlaced") + " (10)";
  }
  if (qAbs(lightCountPercentDiff) > 7 && qAbs(strongCountPercentDiff) > 7) {
    if (firstLightAffinityCountPercent > 10 && firstStrongAffinityCountPercent > 10) {
      infos += " => ";
      return infos + tr("mixed progressive / interlaced") + " (1)";
    }
    infos += " => ";
    return infos + tr("interlaced") + " (11)";
  }
  infos += " => ";
  return infos + tr("progressive");
}
void MPlayerInterlaceDetection::analyse()
{
  int lightAffinity01 = 0, lightAffinity12 = 0, lightAffinity23 = 0;
  int strongAffinity01 = 0, strongAffinity12 = 0, strongAffinity23 = 0;
  int count = 0;
  int duration = 0, lastDuration = 0;
  int telecine32Count = 0;
  int fullLight = 0, fullStrong = 0;
  QStringList affinity, breaks;
  QString tmp;
  bool next = false;
  foreach(QString line, m_data) {
    count++;
    if (line.startsWith("V:")) {
      next = true;
      continue;
    }
    if (!next) {
      continue;
    }
    if (line.startsWith("duration")) {
      tmp = line;
      tmp = tmp.remove(0, tmp.indexOf(":") + 1).trimmed();
      duration = tmp.toInt();
      if (duration == 2 && lastDuration == 3) {
        telecine32Count++;
      }
      lastDuration = duration;
      next = false;
      continue;
    }
    if (line.startsWith("affinity")) {
      if (line.contains("0++")) {
        strongAffinity01++;
      } else if (line.contains("0+")) {
        lightAffinity01++;
      }
      if (line.contains("1++")) {
        strongAffinity12++;
      } else if (line.contains("1+")) {
        lightAffinity12++;
      }
      if (line.contains("2++")) {
        strongAffinity23++;
      } else if (line.contains("2+")) {
        lightAffinity23++;
      }
      if (line.contains("0+.1+.2+.3")) {
        fullLight++;
      } else if (line.contains("0++1++2++3")) {
        fullStrong++;
      }
      tmp = line;
      tmp = tmp.remove(0, tmp.indexOf(":"));
      tmp = tmp.trimmed();
      affinity << tmp;
      continue;
    }
    if (line.startsWith("breaks")) {
      tmp = line;
      tmp = tmp.remove(0, tmp.indexOf(":"));
      tmp = tmp.trimmed();
      breaks << tmp;
      continue;
    }
  }
  if (m_run == 1) {
    m_firstTelecine32Count = telecine32Count;
    m_firstLightAffinity01 = lightAffinity01;
    m_firstLightAffinity12 = lightAffinity12;
    m_firstLightAffinity23 = lightAffinity23;
    m_firstStrongAffinity01 = strongAffinity01;
    m_firstStrongAffinity12 = strongAffinity12;
    m_firstStrongAffinity23 = strongAffinity23;
    m_firstFullLight = fullLight;
    m_firstFullStrong = fullStrong;
    m_firstAffinity = affinity;
    m_firstBreaks = breaks;
    this->startAnalyse();
    return;
  }
  QString out = this->result(lightAffinity01, lightAffinity12, lightAffinity23, strongAffinity01,
                             strongAffinity12, strongAffinity23, telecine32Count, affinity, breaks,
                             count, fullLight, fullStrong);

  m_ui->textBrowser->append(out + "\r\n\r\n");

  if (m_files.isEmpty()) {
    m_ui->label->setText(tr("Analyse is finished!"));
    return;
  }
  m_run = 0;
  this->startAnalyse();
}
