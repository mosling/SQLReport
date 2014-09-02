#ifndef SQLREPORTHIGHLIGHTER_H
#define SQLREPORTHIGHLIGHTER_H

#include <QSyntaxHighlighter>

class SqlReportHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT
	Q_CLASSINFO ("author", "St. Koehler")
	Q_CLASSINFO ("company", "com.github.mosling")

public:
	 explicit SqlReportHighlighter(QTextDocument *parentObj = 0);

 protected:
	 void highlightBlock(const QString &text);

 private:
	 struct HighlightingRule
	 {
		 QRegExp pattern;
		 QTextCharFormat format;
	 };
	 QVector<HighlightingRule> highlightingRules;

	 QRegExp commentStartExpression;
	 QRegExp commentEndExpression;

	 QTextCharFormat singleLineCommentFormat;
	 QTextCharFormat multiLineCommentFormat;
	 QTextCharFormat keywordFormat;
	 QTextCharFormat classFormat;
	 QTextCharFormat substitutionFormat;
	 QTextCharFormat templateFormat;
};

#endif // SQLREPORTHIGHLIGHTER_H
