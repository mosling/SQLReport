#ifndef EDITWIDGET_H
#define EDITWIDGET_H

#include "SqlReportHighlighter.h"

#include <QCloseEvent>
#include <QDialog>
#include <QFile>
#include <QString>
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
	explicit EditWidget(QWidget *parentObj = 0);
    ~EditWidget();

	bool newFile(QString aFileName);
	void saveFile();
	void setLineWrapMode(QTextEdit::LineWrapMode lwp);

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

private:
	bool maybeSave();

    Ui::EditWidget *ui;
	SqlReportHighlighter *highlighter;
	QString currentFileName;
	QString searchString;

};

#endif // EDITWIDGET_H
