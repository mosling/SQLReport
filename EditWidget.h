#ifndef EDITWIDGET_H
#define EDITWIDGET_H

#include "SqlReportHighlighter.h"

#include <QWidget>
#include <QDialog>
#include <QFile>
#include <QCloseEvent>
#include <QString>

namespace Ui {
    class EditWidget;
}

class EditWidget : public QDialog
{
    Q_OBJECT
	Q_CLASSINFO ("author", "St. Koehler")
	Q_CLASSINFO ("company", "com.github.mosling")

public:
	explicit EditWidget(QWidget *parentObj = 0);
    ~EditWidget();

	bool newFile(QString aFileName);
	void saveFile();

protected:
	void keyPressEvent(QKeyEvent *event) override;

protected:
	void closeEvent(QCloseEvent *event);

private slots:
	bool on_btnSave_clicked();
	void on_pushButtonFind_clicked();
	void on_pushButtonPdf_clicked();
	void on_lineEditFind_textChanged(QString str);

private:
	bool maybeSave();

    Ui::EditWidget *ui;
	SqlReportHighlighter *highlighter;
	QString currentFileName;
	QString searchString;

};

#endif // EDITWIDGET_H
