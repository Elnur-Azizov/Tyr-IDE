#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "cevirici.h"
#include "dilmotoru.h"

#include <QToolBar>
#include <QAction>
#include <QSyntaxHighlighter>
#include <QTextCharFormat>
#include <QProcess>
#include <QFileDialog>
#include <QTextStream>
#include <QMessageBox>
#include <QMenuBar>
#include <QStatusBar>
#include <QDockWidget>
#include <QRegularExpression>
#include <QPainter>
#include <QTextBlock>
#include <QTimer>
#include <QLineEdit>
#include <QPushButton>
#include <QHBoxLayout>
#include <QLabel>
#include <QToolButton>
#include <QWidget>
#include <QListWidgetItem>
#include <QTreeWidgetItem>
#include <QTextLayout>
#include <QAbstractTextDocumentLayout>
#include <QMessageBox>
#include <QMessageBox>
#include <QFileDialog>
#include <QTextStream>
#include <QCompleter>
#include <QStringListModel>
#include <QAbstractItemView>
#include <QScrollBar>
#include <QFocusEvent>
#include <QPalette>
#include <QMenu>
#include <QStandardPaths>
#include <QDir>
#include <QFileInfo>
#include <QProcessEnvironment>



// --- RENKLENDİRİCİ SINIFI ---
class TurkceRenklendirici : public QSyntaxHighlighter {
public:
    TurkceRenklendirici(QTextDocument *parent = nullptr) : QSyntaxHighlighter(parent) {
        HighlightingRule rule;

        QTextCharFormat keywordFormat;
        keywordFormat.setForeground(QColor(86, 156, 214));
        keywordFormat.setFontWeight(QFont::Bold);

        QStringList anahtarKelimeler = {
            "tamsayi", "ondalik", "metin", "mantiksal", "donmeyen",
            "eger", "degilse", "tekrarla", "olanadek", "geridondur",
            "dkir", "secim", "durum", "cekirdek", "varsayilan", "ve", "veya"
        };

        for (const QString &pattern : anahtarKelimeler) {
            rule.pattern = QRegularExpression("\\b" + pattern + "\\b");
            rule.format = keywordFormat;
            highlightingRules.append(rule);
        }

        QTextCharFormat numberFormat;
        numberFormat.setForeground(QColor(181, 206, 168));
        rule.pattern = QRegularExpression("\\b\\d+\\b");
        rule.format = numberFormat;
        highlightingRules.append(rule);

        QTextCharFormat commandFormat;
        commandFormat.setForeground(QColor(206, 145, 120));
        QStringList komutlar = { "yazdir", "oku" };

        for (const QString &pattern : komutlar) {
            rule.pattern = QRegularExpression("\\b" + pattern + "\\b");
            rule.format = commandFormat;
            highlightingRules.append(rule);
        }

        QTextCharFormat quotationFormat;
        quotationFormat.setForeground(QColor(106, 153, 85));
        rule.pattern = QRegularExpression("\".*?\"");
        rule.format = quotationFormat;
        highlightingRules.append(rule);
    }

protected:
    void highlightBlock(const QString &text) override {
        for (const HighlightingRule &rule : highlightingRules) {
            QRegularExpressionMatchIterator it = rule.pattern.globalMatch(text);
            while (it.hasNext()) {
                QRegularExpressionMatch match = it.next();
                setFormat(match.capturedStart(), match.capturedLength(), rule.format);
            }
        }
    }

private:
    struct HighlightingRule {
        QRegularExpression pattern;
        QTextCharFormat format;
    };

    QVector<HighlightingRule> highlightingRules;
};

// --- LINE NUMBER AREA ---
LineNumberArea::LineNumberArea(CodeEditor *editor)
    : QWidget(editor), codeEditor(editor) {
}

QSize LineNumberArea::sizeHint() const {
    return QSize(codeEditor->lineNumberAreaWidth(), 0);
}

void LineNumberArea::paintEvent(QPaintEvent *event) {
    codeEditor->lineNumberAreaPaintEvent(event);
}

void LineNumberArea::mousePressEvent(QMouseEvent *event) {
    codeEditor->lineNumberAreaMouseEvent(event);
}

// --- CODE EDITOR ---
CodeEditor::CodeEditor(QWidget *parent) : QPlainTextEdit(parent) {
    lineNumberArea = new LineNumberArea(this);

    connect(this, &CodeEditor::blockCountChanged, this, &CodeEditor::updateLineNumberAreaWidth);
    connect(this, &CodeEditor::updateRequest, this, &CodeEditor::updateLineNumberArea);
    connect(this, &CodeEditor::cursorPositionChanged, this, &CodeEditor::highlightCurrentLine);
    connect(this, &CodeEditor::textChanged, this, &CodeEditor::refreshFolding);

    updateLineNumberAreaWidth(0);
    highlightCurrentLine();
    setTabStopDistance(8 * fontMetrics().horizontalAdvance(' '));
}
QCompleter *CodeEditor::completer() const {
    return c;
}

void CodeEditor::setCompleter(QCompleter *completer) {
    if (c)
        QObject::disconnect(c, nullptr, this, nullptr);

    c = completer;

    if (!c)
        return;

    c->setWidget(this);
    c->setCompletionMode(QCompleter::PopupCompletion);
    c->setCaseSensitivity(Qt::CaseInsensitive);

    connect(c, QOverload<const QString &>::of(&QCompleter::activated),
            this, &CodeEditor::insertCompletion);
}

void CodeEditor::insertCompletion(const QString &completion) {
    if (!c || c->widget() != this)
        return;

    QTextCursor tc = textCursor();
    int extra = completion.length() - c->completionPrefix().length();
    tc.movePosition(QTextCursor::Left);
    tc.movePosition(QTextCursor::EndOfWord);
    tc.insertText(completion.right(extra));
    setTextCursor(tc);
}

QString CodeEditor::textUnderCursor() const {
    QTextCursor tc = textCursor();
    tc.select(QTextCursor::WordUnderCursor);
    return tc.selectedText();
}

void CodeEditor::focusInEvent(QFocusEvent *e) {
    if (c)
        c->setWidget(this);

    QPlainTextEdit::focusInEvent(e);
}
QString MainWindow::aktifDosyaYolu() const
{
    int index = editorTabs->currentIndex();
    if (index >= 0 && index < acikDosyaYollari.size())
        return acikDosyaYollari[index];
    return "";
}

void MainWindow::aktifDosyaYolunuAyarla(const QString &yol)
{
    int index = editorTabs->currentIndex();
    if (index < 0)
        return;

    while (acikDosyaYollari.size() < editorTabs->count())
        acikDosyaYollari.append("");

    acikDosyaYollari[index] = yol;
}

QString MainWindow::aktifSekmeAdi() const
{
    int index = editorTabs->currentIndex();
    if (index >= 0)
        return editorTabs->tabText(index);
    return "Adsiz.tr";
}

CodeEditor* MainWindow::yeniEditorOlustur(const QString &sekmeAdi, const QString &icerik)
{
    CodeEditor *editor = new CodeEditor(this);
    editor->setPlainText(icerik);

    editorTabs->addTab(editor, sekmeAdi);
    editorTabs->setCurrentWidget(editor);

    if (mainStack)
        mainStack->setCurrentWidget(editorTabs);

    new TurkceRenklendirici(editor->document());

    editor->setStyleSheet(
        temaKoyuMu
            ? "QPlainTextEdit { background-color: #1e1e1e; color: #d4d4d4; font-family: 'Consolas'; font-size: 10pt; border: none; selection-background-color: #264f78; }"
            : "QPlainTextEdit { background-color: #ffffff; color: #000000; font-family: 'Consolas'; font-size: 10pt; border: none; selection-background-color: #cce8ff; }"
        );

    editor->highlightCurrentLine();

    QStringList kelimeler;
    kelimeler << "tamsayi"
              << "ondalik"
              << "metin"
              << "mantiksal"
              << "donmeyen"
              << "eger"
              << "degilse"
              << "tekrarla"
              << "olanadek"
              << "geridondur"
              << "yazdir"
              << "oku"
              << "secim"
              << "durum"
              << "varsayilan"
              << "ve"
              << "veya"
              << "cekirdek";

    QCompleter *completer = new QCompleter(kelimeler, editor);
    completer->setCaseSensitivity(Qt::CaseInsensitive);
    completer->setFilterMode(Qt::MatchStartsWith);
    editor->setCompleter(completer);

    acikDosyaYollari.append("");

    connect(editor, &CodeEditor::textChanged, this, &MainWindow::baslatHataTaramasi);

    return editor;
}

void MainWindow::cozumGezgininiGuncelle()
{
    dosyaListesi->clear();

    QTreeWidgetItem *projeItem = new QTreeWidgetItem(QStringList() << "Proje: Týr IDE");

    for (int i = 0; i < editorTabs->count(); ++i) {
        QString sekmeAdi = editorTabs->tabText(i);
        QTreeWidgetItem *dosyaItem = new QTreeWidgetItem(QStringList() << sekmeAdi);
        projeItem->addChild(dosyaItem);
    }

    projeItem->setExpanded(true);
    dosyaListesi->addTopLevelItem(projeItem);
}
void CodeEditor::keyPressEvent(QKeyEvent *e) {
    if (c && c->popup()->isVisible()) {
        switch (e->key()) {
        case Qt::Key_Enter:
        case Qt::Key_Return:
            e->ignore();
            return;

        case Qt::Key_Tab:
            if (c->popup()->currentIndex().isValid()) {
                insertCompletion(c->popup()->currentIndex().data().toString());
                c->popup()->hide();
                return;
            }
            break;

        case Qt::Key_Escape:
            c->popup()->hide();
            e->accept();
            return;

        default:
            break;
        }
    }

    // TAB = 4 space
    if (e->key() == Qt::Key_Tab) {
        insertPlainText("    ");
        return;
    }

    if (e->text() == "{") {
        insertPlainText("{}");
        moveCursor(QTextCursor::Left);
        return;
    }
    else if (e->text() == "(") {
        insertPlainText("()");
        moveCursor(QTextCursor::Left);
        return;
    }
    else if (e->text() == "\"") {
        insertPlainText("\"\"");
        moveCursor(QTextCursor::Left);
        return;
    }

    QPlainTextEdit::keyPressEvent(e);

    if (!c)
        return;

    // Sadece harf, rakam veya _ yazarken öneri çalışsın
    QString txt = e->text();
    if (txt.isEmpty() || !txt.at(0).isLetterOrNumber() && txt != "_") {
        c->popup()->hide();
        return;
    }

    QString completionPrefix = textUnderCursor();

    if (completionPrefix.length() < 2) {
        c->popup()->hide();
        return;
    }

    if (completionPrefix != c->completionPrefix()) {
        c->setCompletionPrefix(completionPrefix);
        c->popup()->setCurrentIndex(c->completionModel()->index(0, 0));
    }

    // Eğer öneri tam olarak yazılan kelimeyse popup gösterme
    if (c->completionCount() == 1 &&
        c->currentCompletion().compare(completionPrefix, Qt::CaseInsensitive) == 0) {
        c->popup()->hide();
        return;
    }

    QRect cr = cursorRect();
    cr.setWidth(c->popup()->sizeHintForColumn(0)
                + c->popup()->verticalScrollBar()->sizeHint().width());

    c->complete(cr);
}

int CodeEditor::lineNumberAreaWidth() {
    int digits = 1;
    int max = qMax(1, blockCount());

    while (max >= 10) {
        max /= 10;
        digits++;
    }

    return 34 + fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits;
}

void CodeEditor::updateLineNumberAreaWidth(int) {
    setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
}

void CodeEditor::updateLineNumberArea(const QRect &rect, int dy) {
    if (dy)
        lineNumberArea->scroll(0, dy);
    else
        lineNumberArea->update(0, rect.y(), lineNumberArea->width(), rect.height());

    if (rect.contains(viewport()->rect()))
        updateLineNumberAreaWidth(0);
}

void CodeEditor::resizeEvent(QResizeEvent *e) {
    QPlainTextEdit::resizeEvent(e);

    QRect cr = contentsRect();
    lineNumberArea->setGeometry(QRect(cr.left(), cr.top(), lineNumberAreaWidth(), cr.height()));
}
CodeEditor* MainWindow::aktifEditor()
{
    return qobject_cast<CodeEditor*>(editorTabs->currentWidget());
}

void CodeEditor::highlightCurrentLine()
{
    QList<QTextEdit::ExtraSelection> extraSelections;

    bool isDark = this->styleSheet().contains("#1e1e1e")
                  || viewport()->palette().color(QPalette::Base).lightness() < 128;

    if (!isReadOnly()) {
        QTextEdit::ExtraSelection selection;
        selection.format.setBackground(isDark ? QColor(60, 60, 60) : QColor(232, 242, 254));
        selection.format.setProperty(QTextFormat::FullWidthSelection, true);
        selection.cursor = textCursor();
        selection.cursor.clearSelection();
        extraSelections.append(selection);
    }

    if (hataSatiri > 0) {
        QTextBlock block = document()->findBlockByNumber(hataSatiri - 1);

        if (block.isValid()) {
            QTextEdit::ExtraSelection errorSel;
            errorSel.format.setBackground(QColor(255, 70, 70, 60));
            errorSel.format.setProperty(QTextFormat::FullWidthSelection, true);

            QTextCursor cursor(block);
            cursor.clearSelection();
            errorSel.cursor = cursor;

            extraSelections.append(errorSel);
        }
    }

    QTextCursor cursor = textCursor();
    int pos = cursor.position();

    auto parantezSecimiEkle = [&](int index) {
        if (index < 0 || index >= document()->characterCount())
            return;

        QTextEdit::ExtraSelection sel;
        sel.format.setBackground(isDark ? QColor(90, 90, 120) : QColor(180, 200, 255));

        QTextCursor c(document());
        c.setPosition(index);
        c.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor);

        sel.cursor = c;
        extraSelections.append(sel);
    };

    auto ileriAra = [&](QChar openCh, QChar closeCh, int startPos) -> int {
        int depth = 1;
        for (int i = startPos; i < document()->characterCount(); ++i) {
            QChar ch = document()->characterAt(i);
            if (ch == openCh)
                depth++;
            else if (ch == closeCh)
                depth--;

            if (depth == 0)
                return i;
        }
        return -1;
    };

    auto geriAra = [&](QChar openCh, QChar closeCh, int startPos) -> int {
        int depth = 1;
        for (int i = startPos; i >= 0; --i) {
            QChar ch = document()->characterAt(i);
            if (ch == closeCh)
                depth++;
            else if (ch == openCh)
                depth--;

            if (depth == 0)
                return i;
        }
        return -1;
    };

    int kontrolPos = -1;
    QChar ch;

    if (pos > 0) {
        QChar onceki = document()->characterAt(pos - 1);
        if (onceki == '(' || onceki == ')' ||
            onceki == '{' || onceki == '}' ||
            onceki == '[' || onceki == ']') {
            kontrolPos = pos - 1;
            ch = onceki;
        }
    }

    if (kontrolPos == -1 && pos < document()->characterCount()) {
        QChar simdiki = document()->characterAt(pos);
        if (simdiki == '(' || simdiki == ')' ||
            simdiki == '{' || simdiki == '}' ||
            simdiki == '[' || simdiki == ']') {
            kontrolPos = pos;
            ch = simdiki;
        }
    }

    if (kontrolPos != -1) {
        int eslesenPos = -1;

        if (ch == '(')
            eslesenPos = ileriAra('(', ')', kontrolPos + 1);
        else if (ch == '{')
            eslesenPos = ileriAra('{', '}', kontrolPos + 1);
        else if (ch == '[')
            eslesenPos = ileriAra('[', ']', kontrolPos + 1);
        else if (ch == ')')
            eslesenPos = geriAra('(', ')', kontrolPos - 1);
        else if (ch == '}')
            eslesenPos = geriAra('{', '}', kontrolPos - 1);
        else if (ch == ']')
            eslesenPos = geriAra('[', ']', kontrolPos - 1);

        parantezSecimiEkle(kontrolPos);
        if (eslesenPos != -1)
            parantezSecimiEkle(eslesenPos);
    }

    setExtraSelections(extraSelections);
}

int CodeEditor::blockNumberAtY(int y) const {
    QTextBlock block = firstVisibleBlock();
    int top = qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
    int bottom = top + qRound(blockBoundingRect(block).height());

    while (block.isValid()) {
        if (y >= top && y <= bottom) {
            return block.blockNumber();
        }

        block = block.next();
        top = bottom;
        bottom = top + qRound(blockBoundingRect(block).height());
    }

    return -1;
}

int CodeEditor::nextNonEmptyBlockNumber(int fromBlockNumber) const {
    QTextBlock block = document()->findBlockByNumber(fromBlockNumber + 1);

    while (block.isValid()) {
        if (!block.text().trimmed().isEmpty())
            return block.blockNumber();
        block = block.next();
    }

    return -1;
}

int CodeEditor::findOpeningBraceBlock(int headerBlockNumber) const {
    QTextBlock header = document()->findBlockByNumber(headerBlockNumber);
    if (!header.isValid())
        return -1;

    QString txt = header.text().trimmed();

    // Eğer header satırında { varsa direkt orası
    if (txt.contains("{"))
        return headerBlockNumber;

    // Header satırının kendisi sadece { veya } olmamalı
    if (txt == "{" || txt == "}")
        return -1;

    // Sonraki anlamlı satır { ise opening odur
    int nextBlockNum = nextNonEmptyBlockNumber(headerBlockNumber);
    if (nextBlockNum < 0)
        return -1;

    QTextBlock nextBlock = document()->findBlockByNumber(nextBlockNum);
    if (nextBlock.text().trimmed() == "{")
        return nextBlockNum;

    return -1;
}


int CodeEditor::findClosingBraceBlock(int openingBraceBlockNumber) const {
    QTextBlock block = document()->findBlockByNumber(openingBraceBlockNumber);
    if (!block.isValid())
        return -1;

    int depth = 0;

    while (block.isValid()) {
        const QString text = block.text();

        for (QChar ch : text) {
            if (ch == '{') depth++;
            if (ch == '}') depth--;

            if (depth == 0 && block.blockNumber() > openingBraceBlockNumber) {
                return block.blockNumber();
            }
        }

        block = block.next();
    }

    return -1;
}

bool CodeEditor::isFoldHeader(int blockNumber) const {
    QTextBlock block = document()->findBlockByNumber(blockNumber);
    if (!block.isValid())
        return false;

    QString txt = block.text().trimmed();
    if (txt.isEmpty())
        return false;

    // Sadece süslü parantez satırları fold başlığı olmasın
    if (txt == "{" || txt == "}")
        return false;

    // Satırın kendisinde hem başlık hem { varsa
    if (txt.contains("{"))
        return true;

    // Sonraki anlamlı satır sadece { ise bu satır başlıktır
    int nextBlockNum = nextNonEmptyBlockNumber(blockNumber);
    if (nextBlockNum < 0)
        return false;

    QTextBlock nextBlock = document()->findBlockByNumber(nextBlockNum);
    QString nextText = nextBlock.text().trimmed();

    return nextText == "{";
}


void CodeEditor::setBlocksHidden(int fromBlockNumber, int toBlockNumber, bool hidden) {
    if (fromBlockNumber > toBlockNumber)
        return;

    QTextBlock block = document()->findBlockByNumber(fromBlockNumber);

    while (block.isValid() && block.blockNumber() <= toBlockNumber) {
        block.setVisible(!hidden);

        if (hidden) {
            block.setLineCount(0);
        } else {
            if (block.layout())
                block.setLineCount(qMax(1, block.layout()->lineCount()));
            else
                block.setLineCount(1);
        }

        block = block.next();
    }

    document()->markContentsDirty(0, document()->characterCount());
    viewport()->update();
    updateLineNumberAreaWidth(0);
}

void CodeEditor::toggleFold(int blockNumber) {
    if (!isFoldHeader(blockNumber))
        return;

    int openingBlock = findOpeningBraceBlock(blockNumber);
    if (openingBlock < 0)
        return;

    int closingBlock = findClosingBraceBlock(openingBlock);
    if (closingBlock < 0 || closingBlock <= openingBlock)
        return;

    bool willCollapse = !collapsedHeaders.contains(blockNumber);

    if (willCollapse) {
        collapsedHeaders.insert(blockNumber);
        setBlocksHidden(openingBlock + 1, closingBlock - 1, true);
    } else {
        collapsedHeaders.remove(blockNumber);
        setBlocksHidden(openingBlock + 1, closingBlock - 1, false);
    }
}

void CodeEditor::refreshFolding() {
    QSet<int> copy = collapsedHeaders;
    collapsedHeaders.clear();

    QTextBlock block = document()->firstBlock();
    while (block.isValid()) {
        block.setVisible(true);
        if (block.layout())
            block.setLineCount(qMax(1, block.layout()->lineCount()));
        else
            block.setLineCount(1);
        block = block.next();
    }

    document()->markContentsDirty(0, document()->characterCount());

    for (int header : copy) {
        if (isFoldHeader(header)) {
            int openingBlock = findOpeningBraceBlock(header);
            int closingBlock = findClosingBraceBlock(openingBlock);
            if (openingBlock >= 0 && closingBlock > openingBlock) {
                collapsedHeaders.insert(header);
                setBlocksHidden(openingBlock + 1, closingBlock - 1, true);
            }
        }
    }

    viewport()->update();
}

void CodeEditor::lineNumberAreaMouseEvent(QMouseEvent *event) {
    int blockNumber = blockNumberAtY(event->pos().y());
    if (blockNumber < 0)
        return;

    if (event->pos().x() <= 16 && isFoldHeader(blockNumber)) {
        toggleFold(blockNumber);
        return;
    }
}

void CodeEditor::lineNumberAreaPaintEvent(QPaintEvent *event) {
    QPainter painter(lineNumberArea);

    bool isDark = this->styleSheet().contains("#1e1e1e")
                  || viewport()->palette().color(QPalette::Base).lightness() < 128;
    painter.fillRect(event->rect(), isDark ? QColor(30, 30, 30) : QColor(243, 243, 243));

    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
    int bottom = top + qRound(blockBoundingRect(block).height());

    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            QString number = QString::number(blockNumber + 1);

            if (isFoldHeader(blockNumber)) {
                QRect foldRect(2, top + 4, 10, 10);

                painter.setPen(isDark ? QColor(170, 170, 170) : QColor(90, 90, 90));
                painter.setBrush(Qt::NoBrush);
                painter.drawRect(foldRect);

                // Yatay çizgi her zaman var
                painter.drawLine(foldRect.left() + 2, foldRect.center().y(),
                                 foldRect.right() - 2, foldRect.center().y());

                // Katlıysa + gibi görünür, açıksa - gibi görünür
                if (collapsedHeaders.contains(blockNumber)) {
                    painter.drawLine(foldRect.center().x(), foldRect.top() + 2,
                                     foldRect.center().x(), foldRect.bottom() - 2);
                }
            }



            if (isDark) {
                painter.setPen(blockNumber == textCursor().blockNumber()
                               ? QColor(220, 220, 220)
                               : QColor(133, 133, 133));
            } else {
                painter.setPen(blockNumber == textCursor().blockNumber()
                               ? QColor(0, 0, 0)
                               : QColor(128, 128, 128));
            }

            painter.drawText(18, top, lineNumberArea->width() - 22,
                             fontMetrics().height(), Qt::AlignRight, number);
        }

        block = block.next();
        top = bottom;
        bottom = top + qRound(blockBoundingRect(block).height());
        ++blockNumber;
    }
}

// --- MAINWINDOW ---
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
    ui(new Ui::MainWindow),
    motor(nullptr),
    dosyaListesi(nullptr),
    ciktiKonsolu(nullptr),
    dockSolution(nullptr),
    dockOutput(nullptr),
    standardToolbar(nullptr),
    hataTimer(nullptr),
    arkaPlanDerleyici(nullptr),
    actTema(nullptr),
    actRun(nullptr),
    temaKoyuMu(true),
    findPanel(nullptr),
    findLineEdit(nullptr),
    matchCountLabel(nullptr) {
    ui->setupUi(this);
    hataSatiriNo = -1;

    motor = new DilMotoru();

    setupVSLayout();
    setupVSMenus();
    temaKoyuMu = (true);
    temaDegistir(temaKoyuMu);


    hataTimer = new QTimer(this);
    hataTimer->setSingleShot(true);

    connect(hataTimer, &QTimer::timeout, this, &MainWindow::hataTaramasiYap);

    statusBar()->showMessage("Yeni dosya oluşturun veya mevcut bir dosya açın.");

    QStringList kelimeler;
    kelimeler << "tamsayi"
              << "ondalik"
              << "metin"
              << "mantiksal"
              << "donmeyen"
              << "eger"
              << "degilse"
              << "tekrarla"
              << "olanadek"
              << "geridondur"
              << "yazdir"
              << "oku"
              << "secim"
              << "durum"
              << "varsayilan"
              << "ve"
              << "veya"
              << "cekirdek";
}

void MainWindow::temizleHataVurgusu() {
    hataSatiriNo = -1;

    CodeEditor *editor = aktifEditor();
    if (!editor)
        return;

    editor->setErrorLine(-1);
}

static QList<QTextEdit::ExtraSelection> hataSecimiOlustur(QPlainTextEdit *editor, int satirNo) {
    QList<QTextEdit::ExtraSelection> secimler;

    if (!editor || satirNo < 1)
        return secimler;

    QTextBlock block = editor->document()->findBlockByNumber(satirNo - 1);
    if (!block.isValid())
        return secimler;

    QTextEdit::ExtraSelection selection;
    selection.format.setBackground(QColor(255, 80, 80, 60));
    selection.format.setProperty(QTextFormat::FullWidthSelection, true);

    QTextCursor cursor(block);
    cursor.clearSelection();

    selection.cursor = cursor;
    secimler.append(selection);

    return secimler;
}
void MainWindow::cozumGezginiSagTik(const QPoint &pos)
{
    QMenu menu;

    if (temaKoyuMu) {
        menu.setStyleSheet(
            "QMenu { background-color: #2d2d30; color: #f1f1f1; border: 1px solid #3e3e42; }"
            "QMenu::item { padding: 6px 24px 6px 24px; }"
            "QMenu::item:selected { background-color: #3e3e42; }"
            );
    } else {
        menu.setStyleSheet(
            "QMenu { background-color: #ffffff; color: #333333; border: 1px solid #dcdcdc; }"
            "QMenu::item { padding: 6px 24px 6px 24px; }"
            "QMenu::item:selected { background-color: #eaeaea; }"
            );
    }

    QMenu *ekleMenu = menu.addMenu("Ekle");

    QAction *yeniDosya = ekleMenu->addAction("Yeni Dosya");

    QAction *ifElse = ekleMenu->addAction("Eger / Degilse");

    QMenu *donguMenu = ekleMenu->addMenu("Dongu");
    QAction *olanadek = donguMenu->addAction("olanadek");
    QAction *tekrarla = donguMenu->addAction("tekrarla");
    QAction *herbiri = donguMenu->addAction("herbiri");

    QAction *secim = menu.exec(dosyaListesi->mapToGlobal(pos));
    if (!secim) return;

    CodeEditor *editor = aktifEditor();
    if (!editor) return;

    QTextCursor cursor = editor->textCursor();

    if (secim == yeniDosya)
    {
        on_actionYeni_triggered();
    }

    else if (secim == ifElse)
    {
        cursor.insertText(
            "eger ()\n"
            "{\n"
            "    \n"
            "}\n"
            "degilse\n"
            "{\n"
            "    \n"
            "}\n"
            );

        cursor.movePosition(QTextCursor::Up);
        editor->setTextCursor(cursor);
    }

    else if (secim == olanadek)
    {
        cursor.insertText(
            "olanadek ()\n"
            "{\n"
            "    \n"
            "}\n"
            );

        cursor.movePosition(QTextCursor::Up);
        editor->setTextCursor(cursor);
    }

    else if (secim == tekrarla)
    {
        cursor.insertText(
            "tekrarla ()\n"
            "{\n"
            "    \n"
            "}\n"
            );

        cursor.movePosition(QTextCursor::Up);
        editor->setTextCursor(cursor);
    }

    else if (secim == herbiri)
    {
        cursor.insertText(
            "herbiri ()\n"
            "{\n"
            "    \n"
            "}\n"
            );

        cursor.movePosition(QTextCursor::Up);
        editor->setTextCursor(cursor);
    }
}

void MainWindow::setupVSLayout() {
    editorTabs = new QTabWidget(this);
    editorTabs->setTabsClosable(true);

    welcomeWidget = new QWidget(this);
    welcomeWidget->setObjectName("welcomeWidget");

    QVBoxLayout *welcomeLayout = new QVBoxLayout(welcomeWidget);
    welcomeLayout->setAlignment(Qt::AlignCenter);
    welcomeLayout->setSpacing(12);

    welcomeTitleLabel = new QLabel("Týr IDE", welcomeWidget);
    welcomeTitleLabel->setAlignment(Qt::AlignCenter);

    welcomeSubtitleLabel = new QLabel("Türkçe Programlama Dili Geliştirme Ortamı", welcomeWidget);
    welcomeSubtitleLabel->setAlignment(Qt::AlignCenter);

    btnYeniDosya = new QPushButton("Yeni Dosya", welcomeWidget);
    btnYeniDosya->setObjectName("welcomeButton");

    btnDosyaAc = new QPushButton("Dosya Aç", welcomeWidget);
    btnDosyaAc->setObjectName("welcomeButton");

    btnYeniDosya->setFixedWidth(160);
    btnDosyaAc->setFixedWidth(160);
    btnYeniDosya->setFixedHeight(36);
    btnDosyaAc->setFixedHeight(36);

    welcomeLayout->addStretch();
    welcomeLayout->addWidget(welcomeTitleLabel);
    welcomeLayout->addWidget(welcomeSubtitleLabel);
    welcomeLayout->addSpacing(20);
    welcomeLayout->addWidget(btnYeniDosya, 0, Qt::AlignCenter);
    welcomeLayout->addWidget(btnDosyaAc, 0, Qt::AlignCenter);
    welcomeLayout->addStretch();

    mainStack = new QStackedLayout;

    QWidget *central = new QWidget(this);
    central->setLayout(mainStack);

    mainStack->addWidget(welcomeWidget);
    mainStack->addWidget(editorTabs);

    setCentralWidget(central);
    mainStack->setCurrentWidget(welcomeWidget);

    connect(btnYeniDosya, &QPushButton::clicked, this, &MainWindow::on_actionYeni_triggered);
    connect(btnDosyaAc, &QPushButton::clicked, this, &MainWindow::on_actionAc_triggered);

    connect(editorTabs, &QTabWidget::tabCloseRequested, this, [this](int index) {
        if (index < 0 || index >= editorTabs->count())
            return;

        QWidget *w = editorTabs->widget(index);
        editorTabs->removeTab(index);

        if (index >= 0 && index < acikDosyaYollari.size())
            acikDosyaYollari.removeAt(index);

        delete w;
        cozumGezgininiGuncelle();

        if (editorTabs->count() == 0 && mainStack) {
            mainStack->setCurrentWidget(welcomeWidget);
        }
    });

    dockSolution = new QDockWidget("Çözüm Gezgini", this);
    dockSolution->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);

    dosyaListesi = new QTreeWidget();
    dosyaListesi->setHeaderHidden(true);

   cozumGezgininiGuncelle();

    dockSolution->setWidget(dosyaListesi);
    addDockWidget(Qt::LeftDockWidgetArea, dockSolution);

    dosyaListesi->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(dosyaListesi, &QTreeWidget::customContextMenuRequested,
            this, &MainWindow::cozumGezginiSagTik);

    dockOutput = new QDockWidget("Çıktı", this);
    dockOutput->setAllowedAreas(Qt::BottomDockWidgetArea);

    ciktiKonsolu = new QListWidget();
    dockOutput->setWidget(ciktiKonsolu);
    addDockWidget(Qt::BottomDockWidgetArea, dockOutput);

    dockSolution->setMinimumWidth(220);
    dockOutput->setMinimumHeight(140);
}

void MainWindow::setupVSMenus() {
    setWindowTitle("Týr IDE");
    showMaximized();

    // Üst bar konteyneri
    QWidget *topBar = new QWidget(this);
    topBar->setObjectName("topBar");

    QHBoxLayout *topLayout = new QHBoxLayout(topBar);
    topLayout->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    topLayout->setContentsMargins(0, 0, 8, 6);
    topLayout->setSpacing(0);

    // Sol: gerçek menü çubuğu
    QMenuBar *mBar = new QMenuBar(topBar);
    mBar->setObjectName("mainMenuBar");
    mBar->setNativeMenuBar(false);

    // --- DOSYA ---
    QMenu *mFile = mBar->addMenu("Dosya");
    mFile->addAction("Yeni", QKeySequence("Ctrl+N"), this, &MainWindow::on_actionYeni_triggered);
    mFile->addAction("Aç", QKeySequence("Ctrl+O"), this, &MainWindow::on_actionAc_triggered);
    mFile->addAction("Kaydet", QKeySequence("Ctrl+S"), this, &MainWindow::on_actionKaydet_triggered);
    mFile->addAction("Farklı Kaydet", QKeySequence("Ctrl+Shift+S"), this, &MainWindow::on_actionFarkliKaydet_triggered);
    mFile->addSeparator();
    mFile->addAction("Çıkış", QKeySequence("Alt+F4"), this, &MainWindow::on_actionCikis_triggered);

    // --- DÜZENLE ---
    QMenu *mEdit = mBar->addMenu("Düzenle");
    mEdit->addAction("Geri Al", QKeySequence::Undo, this, &MainWindow::on_actionGeriAl_triggered);
    mEdit->addAction("İleri Al", QKeySequence::Redo, this, &MainWindow::on_actionIleriAl_triggered);
    mEdit->addSeparator();
    mEdit->addAction("Kes", QKeySequence::Cut, this, &MainWindow::on_actionKes_triggered);
    mEdit->addAction("Kopyala", QKeySequence::Copy, this, &MainWindow::on_actionKopyala_triggered);
    mEdit->addAction("Yapıştır", QKeySequence::Paste, this, &MainWindow::on_actionYapistir_triggered);
    mEdit->addSeparator();
    mEdit->addAction("Tümünü Seç", QKeySequence::SelectAll, this, &MainWindow::on_actionTumunuSec_triggered);
    mEdit->addAction("Bul", QKeySequence("Ctrl+F"), this, &MainWindow::bulPaneliniAc);

    // --- GÖRÜNÜM ---
    QMenu *mView = mBar->addMenu("Görünüm");
    mView->addAction("Bul (Ctrl+F)", QKeySequence("Ctrl+F"), this, &MainWindow::bulPaneliniAc);
    mView->addAction("Temayı Değiştir", this, &MainWindow::on_actionTemaDegistir_triggered);

    QAction *toggleSol = new QAction("Çözüm Gezgini", this);
    toggleSol->setCheckable(true);
    toggleSol->setChecked(true);
    mView->addAction(toggleSol);
    connect(toggleSol, &QAction::toggled, this, &MainWindow::toggleSolutionExplorer);

    QAction *toggleAlt = new QAction("Çıktı Penceresi", this);
    toggleAlt->setCheckable(true);
    toggleAlt->setChecked(true);
    mView->addAction(toggleAlt);
    connect(toggleAlt, &QAction::toggled, this, &MainWindow::toggleOutputWindow);

    // --- PROJE ---
    QMenu *mProject = mBar->addMenu("Proje");
    mProject->addAction("Yeni Proje", this, &MainWindow::on_actionYeniProje_triggered);
    mProject->addAction("Proje Aç", this, &MainWindow::on_actionProjeAc_triggered);
    mProject->addSeparator();
    mProject->addAction("Dosya Ekle", this, &MainWindow::on_actionDosyaEkle_triggered);
    mProject->addAction("Proje Özellikleri", this, &MainWindow::on_actionProjeOzellikleri_triggered);

    // --- DERLE ---
    QMenu *mBuild = mBar->addMenu("Derle");
    mBuild->addAction("Sözdizimi Kontrolü", this, &MainWindow::on_actionSozdizimiKontrolu_triggered);
    mBuild->addAction("Derle ve Çalıştır", QKeySequence("F5"), this, &MainWindow::on_actionCalistir_triggered);
    mBuild->addSeparator();
    mBuild->addAction("Çıktıyı Temizle", this, &MainWindow::on_actionCiktiTemizle_triggered);

    // --- HATA AYIKLA ---
    QMenu *mDebug = mBar->addMenu("Hata Ayıkla");
    mDebug->addAction("Başlat (F5)", QKeySequence("F5"), this, &MainWindow::on_actionCalistir_triggered);
    mDebug->addAction("Çıktıyı Temizle", QKeySequence(), this, &MainWindow::on_actionCiktiTemizle_triggered);

    // --- ARAÇLAR ---
    QMenu *mTools = mBar->addMenu("Araçlar");
    mTools->addAction("Editör Yazı Tipi", this, [this]() {
        statusBar()->showMessage("Yazı tipi ayarı henüz eklenmedi.", 3000);
    });

    // --- YARDIM ---
    QMenu *mHelp = mBar->addMenu("Yardım");
    mHelp->addAction("Türkçe Dil Sözdizimi", this, &MainWindow::on_actionYardimSozdizimi_triggered);
    mHelp->addAction("Hakkında", this, &MainWindow::on_actionHakkinda_triggered);

    mBar->adjustSize();
    mBar->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
    mBar->setFixedWidth(mBar->sizeHint().width());

    topLayout->addWidget(mBar);
    topLayout->addSpacing(6);

    // Sağ butonlar
    QToolButton *btnRun = new QToolButton(topBar);
    btnRun->setObjectName("btnRun");
    btnRun->setText("▶");
    btnRun->setToolTip("Programı çalıştır (F5)");
    btnRun->setCursor(Qt::PointingHandCursor);
    btnRun->setAutoRaise(true);
    btnRun->setFixedSize(40, 28);

    QToolButton *btnTema = new QToolButton(topBar);
    btnTema->setObjectName("btnTema");
    btnTema->setText("🌓");
    btnTema->setToolTip("Tema değiştir");
    btnTema->setCursor(Qt::PointingHandCursor);
    btnTema->setAutoRaise(true);
    btnTema->setFixedSize(32, 28);

    topLayout->addWidget(btnRun);
    topLayout->addSpacing(3);
    topLayout->addWidget(btnTema);

    setMenuWidget(topBar);

    connect(btnRun, &QToolButton::clicked, this, &MainWindow::on_actionCalistir_triggered);
    connect(btnTema, &QToolButton::clicked, this, &MainWindow::on_actionTemaDegistir_triggered);

    standardToolbar = nullptr;
    actRun = nullptr;
    actTema = nullptr;
}

void MainWindow::temaDegistir(bool koyuMu) {
    temaKoyuMu = koyuMu;

    if (actTema) {
        actTema->setText(koyuMu ? "☀" : "☾");
        actTema->setToolTip(koyuMu ? "Açık tema" : "Koyu tema");
    }

    if (koyuMu) {
        this->setStyleSheet("QMainWindow { background-color: #1e1e1e; }");
        this->menuWidget()->setStyleSheet(
            "QWidget#topBar {"
            "   background-color: #2d2d30;"
            "}"
            "QMenuBar#mainMenuBar {"
            "   background-color: #2d2d30;"
            "   color: #f1f1f1;"
            "   border: none;"
            "   font-size: 9pt;"
            "}"
            "QMenuBar#mainMenuBar::item {"
            "   background: transparent;"
            "   padding: 6px 10px;"
            "}"
            "QMenuBar#mainMenuBar::item:selected {"
            "   background-color: #3e3e42;"
            "}"
            "QToolButton#btnRun {"
            "   color: #32cd32;"
            "   background: transparent;"
            "   border: none;"
            "   font-size: 25pt;"
            "   font-weight: bold;"
            "   padding: 2px 4px;"
            "}"
            "QToolButton#btnRun:hover {"
            "   background-color: #3e3e42;"
            "   border-radius: 4px;"
            "}"
            "QToolButton#btnTema {"
            "   color: #ffd54f;"
            "   background: transparent;"
            "   border: none;"
            "   font-size: 13pt;"
            "   font-weight: bold;"
            "   padding: 2px 4px;"
            "}"
            "QToolButton#btnTema:hover {"
            "   background-color: #3e3e42;"
            "   border-radius: 4px;"
            "}"
            );

        editorTabs->setStyleSheet(
            "QTabWidget::pane { border: 1px solid #333; background: #1e1e1e; }"
            "QTabBar::tab { background: #2d2d30; color: #cccccc; padding: 8px 14px; border: 1px solid #333; }"
            "QTabBar::tab:selected { background: #1e1e1e; color: white; }"
            "QTabBar::tab:hover { background: #3e3e42; }"
            );

        statusBar()->setStyleSheet(
            "QStatusBar { background: #007acc; color: white; font-size: 9pt; }"
            );

        QString dockStyle =
            "QDockWidget { color: #f1f1f1; font-weight: bold; font-size: 9pt; }"
            "QDockWidget::title { background: #2d2d30; padding: 6px; }"
            "QTreeWidget, QListWidget { background-color: #252526; color: #cccccc; border: 1px solid #333; font-size: 9pt; }";

        dockSolution->setStyleSheet(dockStyle);
        dockOutput->setStyleSheet(dockStyle);

        if (welcomeWidget) {
            welcomeWidget->setStyleSheet("background-color: #1e1e1e;");

            if (welcomeTitleLabel)
                welcomeTitleLabel->setStyleSheet("color: #f5f5f5; font-size: 28px; font-weight: bold;");

            if (welcomeSubtitleLabel)
                welcomeSubtitleLabel->setStyleSheet("color: #9a9a9a; font-size: 12px;");

            if (btnYeniDosya)
                btnYeniDosya->setStyleSheet(
                    "background-color: #2d2d30;"
                    "color: #f1f1f1;"
                    "border: 1px solid #3e3e42;"
                    "border-radius: 6px;"
                    "padding: 8px 18px;"
                    "font-size: 10pt;"
                    );

            if (btnDosyaAc)
                btnDosyaAc->setStyleSheet(
                    "background-color: #2d2d30;"
                    "color: #f1f1f1;"
                    "border: 1px solid #3e3e42;"
                    "border-radius: 6px;"
                    "padding: 8px 18px;"
                    "font-size: 10pt;"
                    );
        }
    }
    else {
        this->setStyleSheet("QMainWindow { background-color: #ffffff; }");

        this->menuWidget()->setStyleSheet(
            "QWidget#topBar {"
            "   background-color: #ffffff;"
            "}"
            "QMenuBar#mainMenuBar {"
            "   background-color: #ffffff;"
            "   color: #333333;"
            "   border-bottom: 1px solid #dddddd;"
            "   font-size: 9pt;"
            "}"
            "QMenuBar#mainMenuBar::item {"
            "   background: transparent;"
            "   padding: 6px 10px;"
            "}"
            "QMenuBar#mainMenuBar::item:selected {"
            "   background-color: #eeeeee;"
            "}"
            "QToolButton#btnRun {"
            "   color: #1fae4b;"
            "   background: transparent;"
            "   border: none;"
            "   font-size: 25pt;"
            "   font-weight: bold;"
            "   padding: 2px 4px;"
            "}"
            "QToolButton#btnRun:hover {"
            "   background-color: #eaeaea;"
            "   border-radius: 4px;"
            "}"
            "QToolButton#btnTema {"
            "   color: #d4a017;"
            "   background: transparent;"
            "   border: none;"
            "   font-size: 13pt;"
            "   font-weight: bold;"
            "   padding: 2px 4px;"
            "}"
            "QToolButton#btnTema:hover {"
            "   background-color: #eaeaea;"
            "   border-radius: 4px;"
            "}"
            );

        editorTabs->setStyleSheet(
            "QTabWidget::pane { border: 1px solid #dddddd; background: #ffffff; }"
            "QTabBar::tab { background: #f3f3f3; color: #333333; padding: 8px 14px; border: 1px solid #dddddd; }"
            "QTabBar::tab:selected { background: #ffffff; color: #000000; }"
            "QTabBar::tab:hover { background: #eaeaea; }"
            );

        statusBar()->setStyleSheet(
            "QStatusBar { background: #007acc; color: white; font-size: 9pt; }"
            );

        QString dockStyle =
            "QDockWidget { color: #333333; font-weight: bold; font-size: 9pt; }"
            "QDockWidget::title { background: #f3f3f3; padding: 6px; border-bottom: 1px solid #dddddd; }"
            "QTreeWidget, QListWidget { background-color: #f8f8f8; color: #333333; border: 1px solid #eeeeee; font-size: 9pt; }";

        dockSolution->setStyleSheet(dockStyle);
        dockOutput->setStyleSheet(dockStyle);

        if (welcomeWidget) {
            welcomeWidget->setStyleSheet("background-color: #ffffff;");

            if (welcomeTitleLabel)
                welcomeTitleLabel->setStyleSheet("color: #222222; font-size: 28px; font-weight: bold;");

            if (welcomeSubtitleLabel)
                welcomeSubtitleLabel->setStyleSheet("color: #666666; font-size: 12px;");

            if (btnYeniDosya)
                btnYeniDosya->setStyleSheet(
                    "background-color: #f3f3f3;"
                    "color: #222222;"
                    "border: 1px solid #d0d0d0;"
                    "border-radius: 6px;"
                    "padding: 8px 18px;"
                    "font-size: 10pt;"
                    );

            if (btnDosyaAc)
                btnDosyaAc->setStyleSheet(
                    "background-color: #f3f3f3;"
                    "color: #222222;"
                    "border: 1px solid #d0d0d0;"
                    "border-radius: 6px;"
                    "padding: 8px 18px;"
                    "font-size: 10pt;"
                    );
        }
    }

    for (int i = 0; i < editorTabs->count(); ++i) {
        CodeEditor *editor = qobject_cast<CodeEditor*>(editorTabs->widget(i));
        if (!editor)
            continue;

        editor->setStyleSheet(
            koyuMu
                ? "QPlainTextEdit { background-color: #1e1e1e; color: #d4d4d4; font-family: 'Consolas'; font-size: 10pt; border: none; selection-background-color: #264f78; }"
                : "QPlainTextEdit { background-color: #ffffff; color: #000000; font-family: 'Consolas'; font-size: 10pt; border: none; selection-background-color: #cce8ff; }"
            );

        editor->highlightCurrentLine();
    }

    if (findPanel && findLineEdit) {
        findPanel->setStyleSheet(
            koyuMu
                ? "background: #3e3e42; color: white; border: 1px solid #555; border-radius: 4px;"
                : "background: #ffffff; color: black; border: 1px solid #ccc; border-radius: 4px;"
            );
    }
}

void MainWindow::bulPaneliniAc() {
    if (!findPanel) {
        findPanel = new QWidget(this);

        QHBoxLayout *lay = new QHBoxLayout(findPanel);
        lay->setContentsMargins(8, 6, 8, 6);
        lay->setSpacing(6);

        QLabel *iconLabel = new QLabel("🔎", findPanel);

        findLineEdit = new QLineEdit(findPanel);
        findLineEdit->setPlaceholderText("Ara...");

        matchCountLabel = new QLabel("0 sonuç", findPanel);

        QPushButton *btnClose = new QPushButton("X", findPanel);
        btnClose->setFixedSize(22, 22);

        connect(findLineEdit, &QLineEdit::textChanged, this, &MainWindow::metniBul);
        connect(btnClose, &QPushButton::clicked, findPanel, &QWidget::hide);

        lay->addWidget(iconLabel);
        lay->addWidget(findLineEdit);
        lay->addWidget(matchCountLabel);
        lay->addWidget(btnClose);
    }

    temaDegistir(temaKoyuMu);
    findPanel->setGeometry(this->width() - 360, 70, 330, 42);
    findPanel->show();
    findLineEdit->setFocus();
}

void MainWindow::metniBul() {

    CodeEditor *editor = aktifEditor();
    if (!editor)
        return;
    if (!findPanel || !findLineEdit)
        return;

    QString hedef = findLineEdit->text();
    QList<QTextEdit::ExtraSelection> selections;

    if (hedef.isEmpty()) {
        matchCountLabel->setText("0 sonuç");
        matchCountLabel->setStyleSheet("color: #888888;");
        editor->highlightCurrentLine();
        return;
    }

    QTextDocument *doc = editor->document();
    QTextCursor cursor(doc);
    QTextCharFormat highlightFormat;

    QColor vurguRengi = temaKoyuMu ? QColor(100, 255, 100, 100)
                                   : QColor(255, 255, 0, 150);

    highlightFormat.setBackground(vurguRengi);
    highlightFormat.setForeground(Qt::black);

    while (!cursor.isNull() && !cursor.atEnd()) {
        cursor = doc->find(hedef, cursor);
        if (!cursor.isNull()) {
            QTextEdit::ExtraSelection s;
            s.format = highlightFormat;
            s.cursor = cursor;
            selections.append(s);
        }
    }

    editor->setExtraSelections(selections);

    int sayi = selections.count();
    matchCountLabel->setText(QString("%1 sonuç").arg(sayi));
    matchCountLabel->setStyleSheet(
        sayi > 0
            ? "color: #FF8C00; font-weight: bold;"
            : "color: #ff5f56; font-weight: bold;"
        );
}

void MainWindow::on_actionYeni_triggered()
{
    bool ok;
    QString isim = QInputDialog::getText(
        this,
        "Yeni Dosya",
        "Dosya adı:",
        QLineEdit::Normal,
        "",
        &ok
        );

    if (!ok || isim.isEmpty())
        isim = QString("Adsiz-%1.tr").arg(adsizDosyaSayaci++);

    QString gecersizKarakterler = "\\/:*?\"<>|";
    for (QChar ch : isim) {
        if (gecersizKarakterler.contains(ch)) {
            QMessageBox::warning(this, "Geçersiz dosya adı", "Dosya adında geçersiz karakter kullanılamaz.");
            return;
        }
    }

    // .tr ekle
    if (!isim.endsWith(".tr"))
        isim += ".tr";

    for (int i = 0; i < editorTabs->count(); ++i) {
        if (editorTabs->tabText(i) == isim) {
            editorTabs->setCurrentIndex(i);
            statusBar()->showMessage("Bu dosya zaten açık", 2000);
            return;
        }
    }

    yeniEditorOlustur(isim);
    cozumGezgininiGuncelle();
    statusBar()->showMessage("Yeni dosya oluşturuldu", 2000);
}

void MainWindow::on_actionAc_triggered()
{
    QString p = QFileDialog::getOpenFileName(
        this,
        "Dosya Aç",
        "",
        "Týr Dosyaları (*.tr);;Tüm Dosyalar (*.*)"
        );

    if (p.isEmpty())
        return;

    if (!p.endsWith(".tr", Qt::CaseInsensitive))
        p += ".tr";

    QFile f(p);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Hata", "Dosya açılamadı.");
        return;
    }

    QString icerik = QString::fromUtf8(f.readAll());
    f.close();

    QFileInfo info(p);
    yeniEditorOlustur(info.fileName(), icerik);
    aktifDosyaYolunuAyarla(p);
    cozumGezgininiGuncelle();

    statusBar()->showMessage("Dosya açıldı: " + p, 3000);
}

void MainWindow::on_actionKaydet_triggered()
{
    CodeEditor *editor = aktifEditor();
    if (!editor)
        return;

    QString p = aktifDosyaYolu();

    if (p.isEmpty()) {
        on_actionFarkliKaydet_triggered();
        return;
    }

    QFile f(p);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Hata", "Dosya kaydedilemedi.");
        return;
    }

    QTextStream out(&f);
    out << editor->toPlainText();
    f.close();

    statusBar()->showMessage("Dosya kaydedildi: " + p, 3000);
}

void MainWindow::on_actionCikis_triggered() {
    close();
}

void MainWindow::on_actionGeriAl_triggered() {
    CodeEditor *editor = aktifEditor();
    if (!editor)
        return;
    editor->undo();
}

void MainWindow::on_actionIleriAl_triggered() {
    CodeEditor *editor = aktifEditor();
    if (!editor)
        return;
    editor->redo();
}

void MainWindow::on_actionKes_triggered() {
    CodeEditor *editor = aktifEditor();
    if (!editor)
        return;
    editor->cut();
}

void MainWindow::on_actionKopyala_triggered() {
    CodeEditor *editor = aktifEditor();
    if (!editor)
        return;
    editor->copy();
}

void MainWindow::on_actionYapistir_triggered() {
    CodeEditor *editor = aktifEditor();
    if (!editor)
        return;
    editor->paste();
}

void MainWindow::on_actionCiktiTemizle_triggered() {
    ciktiKonsolu->clear();
    statusBar()->showMessage("Çıktı temizlendi", 2000);
}

void MainWindow::toggleSolutionExplorer(bool v) {
    dockSolution->setVisible(v);
}

void MainWindow::toggleOutputWindow(bool v) {
    dockOutput->setVisible(v);
}

void MainWindow::baslatHataTaramasi() {
    CodeEditor *editor = aktifEditor();
    if (!editor)
        return;

    temizleHataVurgusu();
    hataTimer->start(500);
}
void MainWindow::on_actionFarkliKaydet_triggered()
{

    CodeEditor *editor = aktifEditor();
    if (!editor)
        return;

    QString varsayilanAd = aktifSekmeAdi();

    QString p = QFileDialog::getSaveFileName(
        this,
        "Farklı Kaydet",
        varsayilanAd,
        "TürkçeDil Dosyaları (*.tr);;Tüm Dosyalar (*.*)"
        );

    if (p.isEmpty())
        return;

    if (!p.endsWith(".tr", Qt::CaseInsensitive))
        p += ".tr";

    QFile f(p);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Hata", "Dosya kaydedilemedi.");
        return;
    }

    QTextStream out(&f);
    out << editor->toPlainText();
    f.close();

    QFileInfo info(p);
    int index = editorTabs->currentIndex();
    if (index >= 0)
        editorTabs->setTabText(index, info.fileName());

    aktifDosyaYolunuAyarla(p);
    cozumGezgininiGuncelle();

    statusBar()->showMessage("Dosya kaydedildi: " + p, 3000);
}

void MainWindow::on_actionTumunuSec_triggered() {
    CodeEditor *editor = aktifEditor();
    if (!editor)
        return;
    editor->selectAll();
}

void MainWindow::on_actionTemaDegistir_triggered() {
    temaDegistir(!temaKoyuMu);
}

void MainWindow::on_actionSozdizimiKontrolu_triggered() {
    statusBar()->showMessage("Sözdizimi kontrol ediliyor...", 2000);
    hataTaramasiYap();
}

void MainWindow::on_actionYeniProje_triggered() {
    statusBar()->showMessage("Yeni proje oluşturma henüz eklenmedi.", 3000);
}

void MainWindow::on_actionProjeAc_triggered() {
    statusBar()->showMessage("Proje açma özelliği henüz eklenmedi.", 3000);
}

void MainWindow::on_actionDosyaEkle_triggered() {
    statusBar()->showMessage("Dosya ekleme özelliği henüz eklenmedi.", 3000);
}

void MainWindow::on_actionProjeOzellikleri_triggered() {
    statusBar()->showMessage("Proje özellikleri henüz eklenmedi.", 3000);
}

void MainWindow::on_actionYardimSozdizimi_triggered() {
    QMessageBox::information(
        this,
        "Týr Dil Sözdizimi",
        "Örnek kullanım:\n\n"
        "tamsayi cekirdek()\n"
        "{\n"
        "    yazdir \"Merhaba\";\n"
        "}\n\n"
        "Desteklenen bazı anahtar kelimeler:\n"
        "- tamsayi\n"
        "- ondalik\n"
        "- metin\n"
        "- mantiksal\n"
        "- eger\n"
        "- degilse\n"
        "- tekrarla\n"
        "- olanadek\n"
        "- geridondur\n"
        "- yazdir\n"
        "- oku"
        );
}

void MainWindow::on_actionHakkinda_triggered() {
    QMessageBox::information(
        this,
        "Hakkında",
        "Týr IDE\n"
        "Türkçe sözdizimine sahip mini programlama dili editörü ve derleme ortamı.\n\n"
        "Bu uygulama Qt ile geliştirildi."
        );
}


void MainWindow::hataTaramasiYap() {
    hataSatiriNo = -1;
    CodeEditor *editor = aktifEditor();
    if (!editor)
        return;
    editor->setErrorLine(-1);
    if (arkaPlanDerleyici && arkaPlanDerleyici->state() != QProcess::NotRunning) {
        arkaPlanDerleyici->kill();
        arkaPlanDerleyici->waitForFinished(50);
    }
    else if (!arkaPlanDerleyici) {
        arkaPlanDerleyici = new QProcess(this);
    }

    QVector<Parca> p = motor->parcalaraAyir(editor->toPlainText());
    Cevirici c;
    QString cpp = c.turkcedenCppye(p, motor);

    QString appDir = QCoreApplication::applicationDirPath();
    QString gppYolu = appDir + "/mingw1310_64/bin/g++.exe";

    QString mingwBin = appDir + "/mingw1310_64/bin";

    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    QString eskiPath = env.value("PATH");
    env.insert("PATH", QDir::toNativeSeparators(mingwBin) + ";" + eskiPath);

    arkaPlanDerleyici->setProcessEnvironment(env);

    QString calismaKlasoru = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/compiler";
    QDir().mkpath(calismaKlasoru);

    QString tempCppDosya = calismaKlasoru + "/temp_check.cpp";

    QFile f(tempCppDosya);    if (!f.open(QIODevice::WriteOnly | QIODevice::Text))
        return;

    QTextStream out(&f);
    out << cpp;
    f.close();

    disconnect(arkaPlanDerleyici, nullptr, nullptr, nullptr);

    connect(arkaPlanDerleyici,
            QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this,
            [this, editor](int exitCode, QProcess::ExitStatus) {
                ciktiKonsolu->clear();
                temizleHataVurgusu();

                if (exitCode != 0) {
                    QString err = arkaPlanDerleyici->readAllStandardError();
                    const QStringList satirlar = err.split('\n', Qt::SkipEmptyParts);

                    for (const QString &s : satirlar) {
                        if (s.contains("error:")) {
                            QRegularExpression rx("^kaynak\\.tr:(\\d+):(\\d+):\\s*error:\\s*(.*)$");
                            QRegularExpressionMatch match = rx.match(s);

                            QString gosterilecekMesaj;

                            if (match.hasMatch()) {
                                bool satirOk = false;
                                int satirNo = match.captured(1).toInt(&satirOk);
                                QString hataMesaji = match.captured(3).trimmed();

                                if (satirOk && hataSatiriNo == -1) {
                                    hataSatiriNo = satirNo;
                                }

                                if (satirOk) {
                                    gosterilecekMesaj = QString("❌ Satır %1: %2")
                                                            .arg(satirNo)
                                                            .arg(hataMesaji);
                                } else {
                                    gosterilecekMesaj = "❌ " + hataMesaji;
                                }
                            }
                            else {
                                gosterilecekMesaj = "❌ " + s;
                            }

                            QListWidgetItem *it = new QListWidgetItem(gosterilecekMesaj);
                            it->setForeground(QColor(255, 100, 100));
                            ciktiKonsolu->addItem(it);
                            if (match.hasMatch() && hataSatiriNo == -1) {
                                bool ok = false;
                                int satirNo = match.captured(1).toInt(&ok);
                                if (ok) {
                                    hataSatiriNo = satirNo;
                                }
                            }
                        }
                    }
                    if (hataSatiriNo != -1) {
                        editor->setErrorLine(hataSatiriNo);
                    }

                    if (ciktiKonsolu->count() == 0) {
                        ciktiKonsolu->addItem("❌ Sözdizimi hatası bulundu.");
                    }
                }
            });
    arkaPlanDerleyici->setWorkingDirectory(calismaKlasoru);
    arkaPlanDerleyici->start(gppYolu, QStringList() << "-fsyntax-only" << tempCppDosya);

    if (!arkaPlanDerleyici->waitForStarted()) {
        ciktiKonsolu->clear();
        ciktiKonsolu->addItem("✖ HATA: Sözdizimi kontrolü başlatılamadı.");
        ciktiKonsolu->addItem("Gömülü g++ bulunamadı.");
        return;
    }
}
void CodeEditor::paintEvent(QPaintEvent *event)
{
    QPlainTextEdit::paintEvent(event);
    drawIndentGuides(event);
}
void CodeEditor::drawIndentGuides(QPaintEvent *event)
{
    QPainter painter(viewport());

    QColor guideColor(90, 90, 90, 60); // soluk gri
    painter.setPen(guideColor);

    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();

    int top = (int) blockBoundingGeometry(block)
                  .translated(contentOffset()).top();

    int bottom = top + (int) blockBoundingRect(block).height();

    int tabWidth = fontMetrics().horizontalAdvance(' ') * 4;

    while (block.isValid() && top <= event->rect().bottom())
    {
        if (block.isVisible() && bottom >= event->rect().top())
        {
            QString text = block.text();

            int indentLevel = 0;

            for (QChar c : text)
            {
                if (c == ' ')
                    indentLevel++;
                else
                    break;
            }

            indentLevel /= 4;

            for (int i = 1; i <= indentLevel; i++)
            {
                int x = i * tabWidth;

                painter.drawLine(x, top, x, bottom);
            }
        }

        block = block.next();

        top = bottom;
        bottom = top + (int) blockBoundingRect(block).height();
    }
}

void MainWindow::on_actionCalistir_triggered() {
    CodeEditor *editor = aktifEditor();
    if (!editor) {
        statusBar()->showMessage("Açık dosya yok", 2000);
        return;
    }

    QProcess p;

    ciktiKonsolu->clear();
    statusBar()->showMessage("Derleniyor...");

    QVector<Parca> parcalar = motor->parcalaraAyir(editor->toPlainText());
    Cevirici tr;
    QString cppKod = tr.turkcedenCppye(parcalar, motor);

    QString appDir = QCoreApplication::applicationDirPath();
    QString gppYolu = appDir + "/mingw1310_64/bin/g++.exe";

    QString mingwBin = appDir + "/mingw1310_64/bin";

    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    QString eskiPath = env.value("PATH");
    env.insert("PATH", QDir::toNativeSeparators(mingwBin) + ";" + eskiPath);

    p.setProcessEnvironment(env);

    QString calismaKlasoru = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/compiler";
    QDir().mkpath(calismaKlasoru);

    QString cppDosya = calismaKlasoru + "/program.cpp";
    QString exeDosya = calismaKlasoru + "/program.exe";

    QFile::remove(exeDosya);
    QFile::remove(cppDosya);

    QFile f(cppDosya);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        ciktiKonsolu->addItem("✖ HATA: program.cpp oluşturulamadı.");
        statusBar()->showMessage("Dosya oluşturulamadı", 3000);
        return;
    }

    QTextStream out(&f);
    out << cppKod;
    f.close();

    p.setWorkingDirectory(calismaKlasoru);
    p.start(gppYolu, QStringList() << cppDosya << "-o" << exeDosya);

    if (!p.waitForStarted()) {
        ciktiKonsolu->clear();
        ciktiKonsolu->addItem("✖ HATA: Derleyici başlatılamadı.");
        ciktiKonsolu->addItem("Gömülü g++ bulunamadı.");
        statusBar()->showMessage("Derleyici başlatılamadı", 3000);
        return;
    }

    p.waitForFinished(-1);

    if (p.exitCode() == 0 && QFile::exists(exeDosya)) {
        ciktiKonsolu->addItem("✔ Derleme başarılı.");
        statusBar()->showMessage("Derleme başarılı", 3000);

        QString komut = QString("start cmd /C \"\"%1\" & echo. & pause\"").arg(QDir::toNativeSeparators(exeDosya));
        system(komut.toLocal8Bit().constData());
    } else {
        ciktiKonsolu->addItem("✖ HATA: Derleme başarısız.");
        QString hata = QString::fromLocal8Bit(p.readAllStandardError());
        if (hata.trimmed().isEmpty())
            hata = "Derleyici hata mesajı döndürmedi.";
        ciktiKonsolu->addItem(hata);
        statusBar()->showMessage("Derleme başarısız", 3000);
    }
}

MainWindow::~MainWindow() {
    delete ui;
}
