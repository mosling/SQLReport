#ifndef EDITWIDGET_H
#define EDITWIDGET_H

#include "SqlReportHighlighter.h"

#include <QDialog>
#include <QFile>
#include <QCloseEvent>

namespace Ui {
    class EditWidget;
}

class EditWidget : public QDialog
{
    Q_OBJECT

public:
	explicit EditWidget(QWidget *parentObj = 0);
    ~EditWidget();

	void newFile(QString aFileName);
	void saveFile();
	void reject() override;

protected:
	void closeEvent(QCloseEvent *event);

private slots:
	bool on_btnSave_clicked();

private:
	bool maybeSave();

    Ui::EditWidget *ui;
	SqlReportHighlighter *highlighter;
	QString currentFileName;

};

#endif // EDITWIDGET_H
