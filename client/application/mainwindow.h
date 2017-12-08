#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "client.h"

#include <QMainWindow>
#include <QPlainTextEdit>

QT_BEGIN_NAMESPACE
class QAction;
class QMenu;
class QSessionManager;
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow();

    void loadFile(const QString &fileName);

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void newFile();
    void open();
    bool save();
    bool saveAs();
    void about();
    void documentWasModified();
#ifndef QT_NO_SESSIONMANAGER
    void commitData(QSessionManager &);
#endif
    void change_character_received_slot(int position, QString text);
    void pull_document_received_slot(QString text);

public slots:
    void onTextChangedSignal();
private:
    void createActions();
    void createStatusBar();
    void readSettings();
    void writeSettings();
    bool maybeSave();
    bool saveFile(const QString &fileName);
    void setCurrentFile(const QString &fileName);
    QString strippedName(const QString &fullFileName);

    QPlainTextEdit *textEdit;
    QString curFile;
    client m_client;
    std::atomic<bool> isEditedManually;
    int textLength;
};

#endif
