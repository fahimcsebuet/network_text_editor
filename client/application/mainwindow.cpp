#include <QtWidgets>

#include "mainwindow.h"

MainWindow::MainWindow()
    : textEdit(new QPlainTextEdit)
{
    isEditedManually = true;
    setCentralWidget(textEdit);

    createActions();
    createStatusBar();

    readSettings();

    connect(textEdit->document(), &QTextDocument::contentsChanged,
            this, &MainWindow::documentWasModified);

    connect(textEdit, SIGNAL(textChanged()), this, SLOT(onTextChangedSignal()));
    connect(&m_client, SIGNAL(change_character_received_signal(int, QString)),
            this, SLOT(change_character_received_slot(int, QString)));
    connect(&m_client, SIGNAL(pull_document_received_signal(QString)),
            this, SLOT(pull_document_received_slot(QString)));
    connect(&m_client, SIGNAL(exit_signal()), this, SLOT(client_exit_slot()));

#ifndef QT_NO_SESSIONMANAGER
    QGuiApplication::setFallbackSessionManagementEnabled(false);
    connect(qApp, &QGuiApplication::commitDataRequest,
            this, &MainWindow::commitData);
#endif

    setCurrentFile(QString());
    setUnifiedTitleAndToolBarOnMac(true);

    std::string _configuration_file = "configuration_file";

    m_client.init(_configuration_file);
    m_client.start();
    textLength = textEdit->toPlainText().length();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    m_client._exit();
    if (maybeSave()) {
        writeSettings();
        event->accept();
    } else {
        event->ignore();
    }
}

void MainWindow::change_character_received_slot(int position, QString text)
{
    isEditedManually = false;
    QTextCursor _currentCursor = textEdit->textCursor();
    int _currentPosition = _currentCursor.position();
    QTextCursor _newCursor = textEdit->textCursor();
    if(position > _currentPosition)
    {
        _newCursor.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor,
                                position - _currentPosition);
    }
    if(position < _currentPosition)
    {
        _newCursor.movePosition(QTextCursor::Left, QTextCursor::MoveAnchor,
                                _currentPosition - position);
    }
    if(text == "del")
    {
        _newCursor.deleteChar();
    }
    else
    {
        _newCursor.insertText(text);
    }
    //textEdit->setTextCursor(_currentCursor);
}

void MainWindow::pull_document_received_slot(QString text)
{
    textEdit->setPlainText(text);
}

void MainWindow::client_exit_slot()
{
    char _sentinel = -1;
    std::string _command = "pu";
    QString _text = textEdit->toPlainText();
    m_client.send_data_to_server(_command + _sentinel + _text.toStdString());
}

void MainWindow::onTextChangedSignal()
{
    if(!isEditedManually)
    {
        isEditedManually = true;
        return;
    }
    char _sentinel = -1;
    int _position = textEdit->textCursor().position();
    if(_position >= 0)
    {
        std::string _command ="cc"; // Character Change
        std::string _positionString = std::to_string(_position);
        std::string _inputString = "";
        if(textEdit->toPlainText().length() > textLength && _position > 0)
        {
            QChar _charAtCursor = textEdit->toPlainText().at(_position - 1);
            _inputString = QString(_charAtCursor).toStdString();
        }
        else if(textEdit->toPlainText().length() < textLength)
        {
            _inputString = "del";
        }

        std::string _dataToServer = _command + _sentinel + _positionString + _sentinel + _inputString;
        m_client.send_data_to_server(_dataToServer);
        textLength = textEdit->toPlainText().length();
    }
}

void MainWindow::newFile()
{
    if (maybeSave()) {
        textEdit->clear();
        setCurrentFile(QString());
    }
}

void MainWindow::open()
{
    if (maybeSave()) {
        QString fileName = QFileDialog::getOpenFileName(this);
        if (!fileName.isEmpty())
            loadFile(fileName);
    }
}

bool MainWindow::save()
{
    if (curFile.isEmpty()) {
        return saveAs();
    } else {
        return saveFile(curFile);
    }
}

bool MainWindow::saveAs()
{
    QFileDialog dialog(this);
    dialog.setWindowModality(Qt::WindowModal);
    dialog.setAcceptMode(QFileDialog::AcceptSave);
    if (dialog.exec() != QDialog::Accepted)
        return false;
    return saveFile(dialog.selectedFiles().first());
}

void MainWindow::about()
{
   QMessageBox::about(this, tr("About Application"),
            tr("This application is the Client user interface of the software <b>Network Text Editor</b>"));
}

void MainWindow::documentWasModified()
{
    setWindowModified(textEdit->document()->isModified());
}

void MainWindow::createActions()
{
    QMenu *fileMenu = menuBar()->addMenu(tr("&File"));
    QToolBar *fileToolBar = addToolBar(tr("File"));

    const QIcon saveIcon = QIcon::fromTheme("document-save", QIcon(":/images/save.png"));
    QAction *saveAct = new QAction(saveIcon, tr("&Save"), this);
    saveAct->setShortcuts(QKeySequence::Save);
    saveAct->setStatusTip(tr("Save the document to disk"));
    connect(saveAct, &QAction::triggered, this, &MainWindow::save);
    fileMenu->addAction(saveAct);
    fileToolBar->addAction(saveAct);

    const QIcon saveAsIcon = QIcon::fromTheme("document-save-as");
    QAction *saveAsAct = fileMenu->addAction(saveAsIcon, tr("Save &As..."), this, &MainWindow::saveAs);
    saveAsAct->setShortcuts(QKeySequence::SaveAs);
    saveAsAct->setStatusTip(tr("Save the document under a new name"));

    fileMenu->addSeparator();

    const QIcon exitIcon = QIcon::fromTheme("application-exit");
    QAction *exitAct = fileMenu->addAction(exitIcon, tr("E&xit"), this, &QWidget::close);
    exitAct->setShortcuts(QKeySequence::Quit);

    exitAct->setStatusTip(tr("Exit the application"));

    menuBar()->addSeparator();

    QMenu *helpMenu = menuBar()->addMenu(tr("&Help"));
    QAction *aboutAct = helpMenu->addAction(tr("&About"), this, &MainWindow::about);
    aboutAct->setStatusTip(tr("Show the application's About box"));
}

void MainWindow::createStatusBar()
{
    statusBar()->showMessage(tr("Ready"));
}

void MainWindow::readSettings()
{
    QSettings settings(QCoreApplication::organizationName(), QCoreApplication::applicationName());
    const QByteArray geometry = settings.value("geometry", QByteArray()).toByteArray();
    if (geometry.isEmpty()) {
        const QRect availableGeometry = QApplication::desktop()->availableGeometry(this);
        resize(availableGeometry.width() / 3, availableGeometry.height() / 2);
        move((availableGeometry.width() - width()) / 2,
             (availableGeometry.height() - height()) / 2);
    } else {
        restoreGeometry(geometry);
    }
}

void MainWindow::writeSettings()
{
    QSettings settings(QCoreApplication::organizationName(), QCoreApplication::applicationName());
    settings.setValue("geometry", saveGeometry());
}

bool MainWindow::maybeSave()
{
    if (!textEdit->document()->isModified())
        return true;
    const QMessageBox::StandardButton ret
        = QMessageBox::warning(this, tr("Application"),
                               tr("The document has been modified.\n"
                                  "Do you want to save your changes?"),
                               QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
    switch (ret) {
    case QMessageBox::Save:
        return save();
    case QMessageBox::Cancel:
        return false;
    default:
        break;
    }
    return true;
}

void MainWindow::loadFile(const QString &fileName)
{
    QFile file(fileName);
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        QMessageBox::warning(this, tr("Application"),
                             tr("Cannot read file %1:\n%2.")
                             .arg(QDir::toNativeSeparators(fileName), file.errorString()));
        return;
    }

    QTextStream in(&file);
#ifndef QT_NO_CURSOR
    QApplication::setOverrideCursor(Qt::WaitCursor);
#endif
    textEdit->setPlainText(in.readAll());
#ifndef QT_NO_CURSOR
    QApplication::restoreOverrideCursor();
#endif

    setCurrentFile(fileName);
    statusBar()->showMessage(tr("File loaded"), 2000);
}

bool MainWindow::saveFile(const QString &fileName)
{
    QFile file(fileName);
    if (!file.open(QFile::WriteOnly | QFile::Text)) {
        QMessageBox::warning(this, tr("Application"),
                             tr("Cannot write file %1:\n%2.")
                             .arg(QDir::toNativeSeparators(fileName),
                                  file.errorString()));
        return false;
    }

    QTextStream out(&file);
#ifndef QT_NO_CURSOR
    QApplication::setOverrideCursor(Qt::WaitCursor);
#endif
    out << textEdit->toPlainText();
#ifndef QT_NO_CURSOR
    QApplication::restoreOverrideCursor();
#endif

    setCurrentFile(fileName);
    statusBar()->showMessage(tr("File saved"), 2000);
    return true;
}

void MainWindow::setCurrentFile(const QString &fileName)
{
    curFile = fileName;
    textEdit->document()->setModified(false);
    setWindowModified(false);

    QString shownName = curFile;
    if (curFile.isEmpty())
        shownName = "untitled.txt";
    setWindowFilePath(shownName);
}

QString MainWindow::strippedName(const QString &fullFileName)
{
    return QFileInfo(fullFileName).fileName();
}

#ifndef QT_NO_SESSIONMANAGER
void MainWindow::commitData(QSessionManager &manager)
{
    if (manager.allowsInteraction()) {
        if (!maybeSave())
            manager.cancel();
    } else {
        // Non-interactive: save without asking
        if (textEdit->document()->isModified())
            save();
    }
}
#endif
