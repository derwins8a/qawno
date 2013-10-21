#include <QPainter>
#include <QSettings>
#include <QTextEdit>
#include <QTextBlock>
#include <QTextCursor>

#include "EditorWidget.h"
#include "SyntaxHighlighter.h"

LineNumberArea::LineNumberArea(EditorWidget *edit)
  : QWidget(edit),
    editorWidget_(edit)
{
}

QSize LineNumberArea::sizeHint() const {
  int width = editorWidget_->lineNumberAreaWidth();
  return QSize(width, 0);
}

void LineNumberArea::paintEvent(QPaintEvent *paintEvent) {
  editorWidget_->lineNumberAreaPaintEvent(paintEvent);
}

EditorWidget::EditorWidget(QWidget *parent)
  : QPlainTextEdit(parent),
    tabStop_(4)
{
  setLineWrapMode(NoWrap);
  setUndoRedoEnabled(true);

  lineNumberArea_ = new LineNumberArea(this);

  connect(this, SIGNAL(blockCountChanged(int)),
          this, SLOT(updateLineNumberAreaWidth(int)));
  connect(this, SIGNAL(updateRequest(QRect, int)),
          this, SLOT(updateLineNumberArea(QRect, int)));
  connect(this, SIGNAL(cursorPositionChanged()),
          this, SLOT(highlightCurrentLine()));

  highlightCurrentLine();
  updateLineNumberAreaWidth(0);

  highlighter_ = new SyntaxHighlighter(this);
  highlighter_->setDocument(document());

  QSettings settings;
  settings.beginGroup("Widgets");
    settings.beginGroup("Editor");
      settings.beginGroup("Font");
        QFont font;
        font.setFamily(settings.value("Family", "Courier New").toString());
        font.setPointSize(settings.value("PointSize", 10).toInt());
        font.setBold(settings.value("Bold", false).toBool());
        font.setItalic(settings.value("Italic", false).toBool());
        QPlainTextEdit::setFont(font);
      settings.endGroup();
    settings.endGroup();
  settings.endGroup();

  setTabStopWidth(fontMetrics().width(' ') * tabStop_);
}

EditorWidget::~EditorWidget() {
  QSettings settings;
  settings.beginGroup("Widgets");
    settings.beginGroup("Editor");
      settings.beginGroup("Font");
        settings.setValue("Family", font().family());
        settings.setValue("PointSize", font().pointSize());
        settings.setValue("Bold", font().bold());
        settings.setValue("Italic", font().italic());
      settings.endGroup();
    settings.endGroup();
  settings.endGroup();
}

void EditorWidget::setCurrentLine(long line) {
  if (line > 0 && line <= document()->blockCount()) {
    QTextCursor cur = this->textCursor();
    int pos = document()->findBlockByLineNumber(line - 1).position();
    cur.setPosition(pos);
    setTextCursor(cur);
  }
}

int EditorWidget::tabStop() const {
  return tabStop_;
}

void EditorWidget::setTabStop(int chars) {
  tabStop_ = chars;
}

void EditorWidget::lineNumberAreaPaintEvent(QPaintEvent *paintEvent) {
  QPainter painter(lineNumberArea_);
  painter.fillRect(paintEvent->rect(), Qt::lightGray);

  QTextBlock block = firstVisibleBlock();
  int blockNumber = block.blockNumber();
  int top = (int)blockBoundingGeometry(block).translated(contentOffset()).top();
  int bottom = top + (int) blockBoundingRect(block).height();

  while (block.isValid() && top <= paintEvent->rect().bottom()) {
    if (block.isVisible() && bottom >= paintEvent->rect().top()) {
      QString number = QString::number(blockNumber + 1);
      painter.setPen(Qt::black);
      painter.drawText(lineNumberArea_->width() * 0.25,
      top,  lineNumberArea_->width() * 0.5,
      fontMetrics().height(), Qt::AlignRight, number);
    }

    block = block.next();
    top = bottom;
    bottom = top + (int) blockBoundingRect(block).height();
    ++blockNumber;
  }
}

int EditorWidget::lineNumberAreaWidth() const {
  int numDigits = 1;
  int max = qMax(1, blockCount());

  while (max >= 10) {
    max /= 10;
    ++numDigits;
  }

  int digitWidth = fontMetrics().width(QLatin1Char('9'));
  int spaceRequired = digitWidth * numDigits;

  // We will leave some space blank for the number's left and right
  // margins (0.5 of number width for both, see lineNumberAreaPaintEvent)
  int actualSpace = spaceRequired * 2;

  return actualSpace;
}

void EditorWidget::resizeEvent(QResizeEvent *resizeEvent) {
  QPlainTextEdit::resizeEvent(resizeEvent);

  QRect cr = contentsRect();
  lineNumberArea_->setGeometry(QRect(cr.left(), cr.top(),
  lineNumberAreaWidth(), cr.height()));
}

void EditorWidget::updateLineNumberArea(const QRect &rect, int dy) {
  if (dy) {
    lineNumberArea_->scroll(0, dy);
  } else {
    lineNumberArea_->update(0, rect.y(),
                            lineNumberArea_->width(), rect.height());
  }

  if (rect.contains(viewport()->rect())) {
    updateLineNumberAreaWidth(0);
  }
}

void EditorWidget::updateLineNumberAreaWidth(int) {
  setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
}

void EditorWidget::highlightCurrentLine() {
  QList<QTextEdit::ExtraSelection> extraSelections;

  if (!isReadOnly()) {
    QTextEdit::ExtraSelection selection;
    QColor lineColor = QColor(Qt::lightGray).lighter(120);
    selection.format.setBackground(lineColor);
    selection.format.setProperty(QTextFormat::FullWidthSelection, true);
    selection.cursor = textCursor();
    selection.cursor.clearSelection();
    extraSelections.append(selection);
  }

  setExtraSelections(extraSelections);
}