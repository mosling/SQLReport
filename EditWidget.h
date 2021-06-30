#ifndef EDITWIDGET_H
#define EDITWIDGET_H

#include "SqlReportHighlighter.h"

#include <QCloseEvent>
#include <QDialog>
#include <QFile>
#include <QString>
#include <QStringListModel>
#include <QTextEdit>
#include <QWidget>

namespace Ui {
    class EditWidget;
}

class EditWidget : public QWidget
{
    Q_OBJECT
	Q_CLASSINFO ("author", "St. Koehler")
	Q_CLASSINFO ("company", "com.github.mosling")

public:
    explicit EditWidget(QWidget *parentObj, bool showToc);
    ~EditWidget();

	bool newFile(QString aFileName);
	void saveFile();
	void setLineWrapMode(QTextEdit::LineWrapMode lwp);
    void placeCursorAtNode(QString nodeName);
    void setConnectedWidget(EditWidget *w) { connectedWidget = w; }

protected:
	void keyPressEvent(QKeyEvent *event) override;

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
	bool on_btnSave_clicked();
    bool on_btnSaveAs_clicked();
	void on_pushButtonFind_clicked();
	void on_pushButtonPdf_clicked();
	void on_lineEditFind_textChanged(QString str);
	void on_lineEditFind_returnPressed();
	void on_pushButtonWrap_toggled(bool b);

    void on_lvToc_clicked(const QModelIndex &index);

    void on_teEditor_textChanged();

private:
	bool maybeSave();
    void updateTableOfContent();
    void updateCursorNode(QString nodeName);

    Ui::EditWidget *ui;
	SqlReportHighlighter *highlighter;
	QString currentFileName;
	QString searchString;
    QStringListModel *tableOfContent;
    EditWidget *connectedWidget;

};

#endif // EDITWIDGET_H
