#include <QApplication>

#include "MPlayerInterlaceDetection.h"

int main(int argc, char *argv[])
{
  QApplication application(argc, argv);
  MPlayerInterlaceDetection mainWindow;
  mainWindow.show();
  return application.exec();
}
