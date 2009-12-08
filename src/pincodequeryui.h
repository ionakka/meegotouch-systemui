#ifndef  PINCODEQUERYUI_H
#define  PINCODEQUERYUI_H

#include <DuiApplicationPage>

class DuiButton;
class DuiLabel;
class DuiTextEdit;
class DuiLayout;
class DuiGridLayoutPolicy;
class DuiSceneManager;
class QTimer;
class QStringList;

class  PinCodeQueryUI : public DuiApplicationPage
{
    Q_OBJECT

public:
    PinCodeQueryUI(QStringList emergencyNumbers);
    virtual ~ PinCodeQueryUI();
    virtual void createContent();

    DuiButton *getEmergencyBtn();
    DuiButton *getCancelBtn();
    DuiButton *getEnterBtn();
    DuiTextEdit *getCodeEntry();

    void setHeader(QString);
    void checkEntry();
    static void hideWindow();
    static void showWindow();
    static void setWindowOnTop(bool onTop = true);

public slots:
    virtual void appear(DuiSceneWindow::DeletionPolicy policy=KeepWhenDone);
    virtual void appearNow(DuiSceneWindow::DeletionPolicy policy=KeepWhenDone);
    virtual void disappear();
    virtual void disappearNow();

private slots:
    void buttonReleased();
    void buttonPressed();
    void removeText();

private: //methods
    void createWidgetItems();
    QGraphicsWidget* createNumpad();

private: //attributes
    DuiButton *emergencyCallButton;
    DuiButton *enterButton;
    DuiButton *cancelButton;
    DuiButton *backspaceButton;
    DuiLabel *headerLabel;
    DuiTextEdit *entryTextEdit;
    QTimer *backspaceTimer;
    QStringList emergencyNumbers;
};

#endif //  PINCODEQUERYUI_H
