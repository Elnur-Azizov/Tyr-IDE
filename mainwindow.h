#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QWidget>
#include <QPlainTextEdit>
#include <QTreeWidget>
#include <QListWidget>
#include <QDockWidget>
#include <QPainter>
#include <QTextBlock>
#include <QProcess>
#include <QTimer>
#include <QKeyEvent>
#include <QLineEdit>
#include <QPushButton>
#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>
#include <QToolBar>
#include <QToolButton>
#include <QAction>
#include <QPaintEvent>
#include <QResizeEvent>
#include <QRect>
#include <QSize>
#include <QSet>
#include <QMouseEvent>
#include <QCompleter>
#include <QStringListModel>
#include <QFocusEvent>
#include <QTabWidget>
#include "dilmotoru.h"
#include <QStackedLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QInputDialog>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class CodeEditor;

class LineNumberArea : public QWidget
{
public:
    explicit LineNumberArea(CodeEditor *editor);
    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;

private:
    CodeEditor *codeEditor;
};

class CodeEditor : public QPlainTextEdit
{
    Q_OBJECT

public:
    explicit CodeEditor(QWidget *parent = nullptr);

    void lineNumberAreaPaintEvent(QPaintEvent *event);
    void lineNumberAreaMouseEvent(QMouseEvent *event);
    int lineNumberAreaWidth();
    void highlightCurrentLine();
    void refreshFolding();

    void setCompleter(QCompleter *completer);
    QCompleter *completer() const;

    void setErrorLine(int line) {
        hataSatiri = line;
        highlightCurrentLine();
    }

protected:
    void resizeEvent(QResizeEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void focusInEvent(QFocusEvent *e) override;
    void paintEvent(QPaintEvent *event) override;

private slots:
    void updateLineNumberAreaWidth(int newBlockCount);
    void updateLineNumberArea(const QRect &rect, int dy);
    void insertCompletion(const QString &completion);

private:
    void drawIndentGuides(QPaintEvent *event);
    QWidget *lineNumberArea;
    QSet<int> collapsedHeaders;

    QCompleter *c = nullptr;
    int hataSatiri = -1;

    QString textUnderCursor() const;

    int blockNumberAtY(int y) const;
    int nextNonEmptyBlockNumber(int fromBlockNumber) const;
    int findOpeningBraceBlock(int headerBlockNumber) const;
    int findClosingBraceBlock(int openingBraceBlockNumber) const;

    bool isFoldHeader(int blockNumber) const;
    void toggleFold(int blockNumber);
    void setBlocksHidden(int fromBlockNumber, int toBlockNumber, bool hidden);
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_actionYeni_triggered();
    void on_actionAc_triggered();
    void on_actionKaydet_triggered();
    void on_actionFarkliKaydet_triggered();
    void on_actionCikis_triggered();

    void on_actionGeriAl_triggered();
    void on_actionIleriAl_triggered();
    void on_actionKes_triggered();
    void on_actionKopyala_triggered();
    void on_actionYapistir_triggered();
    void on_actionTumunuSec_triggered();

    void toggleSolutionExplorer(bool v);
    void toggleOutputWindow(bool v);

    void temaDegistir(bool koyuMu);
    void on_actionTemaDegistir_triggered();

    void on_actionCalistir_triggered();
    void on_actionCiktiTemizle_triggered();
    void on_actionSozdizimiKontrolu_triggered();
    void temizleHataVurgusu();

    void hataTaramasiYap();
    void baslatHataTaramasi();

    void bulPaneliniAc();
    void metniBul();
    void cozumGezginiSagTik(const QPoint &pos);

    void on_actionYeniProje_triggered();
    void on_actionProjeAc_triggered();
    void on_actionDosyaEkle_triggered();
    void on_actionProjeOzellikleri_triggered();

    void on_actionYardimSozdizimi_triggered();
    void on_actionHakkinda_triggered();

private:
    void setupVSLayout();
    void setupVSMenus();
    CodeEditor* aktifEditor();

private:
    QStringList acikDosyaYollari;
    int adsizDosyaSayaci = 1;

    QString aktifDosyaYolu() const;
    void aktifDosyaYolunuAyarla(const QString &yol);
    QString aktifSekmeAdi() const;
    void cozumGezgininiGuncelle();
    CodeEditor* yeniEditorOlustur(const QString &sekmeAdi, const QString &icerik = "");
    int hataSatiriNo;
    Ui::MainWindow *ui;

    QTabWidget *editorTabs;
    DilMotoru *motor;

    QTreeWidget *dosyaListesi;
    QListWidget *ciktiKonsolu;

    QDockWidget *dockSolution;
    QDockWidget *dockOutput;

    QToolBar *standardToolbar;

    QTimer *hataTimer;
    QProcess *arkaPlanDerleyici;

    QAction *actTema;
    QAction *actRun;

    bool temaKoyuMu;

    QWidget *findPanel;
    QLineEdit *findLineEdit;
    QLabel *matchCountLabel;
    QWidget *welcomeWidget;
    QPushButton *btnYeniDosya;
    QPushButton *btnDosyaAc;
    QStackedLayout *mainStack;
    QLabel *welcomeTitleLabel;
    QLabel *welcomeSubtitleLabel;

};

#endif // MAINWINDOW_H
