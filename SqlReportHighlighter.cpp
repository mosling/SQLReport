#include "SqlReportHighlighter.h"

SqlReportHighlighter::SqlReportHighlighter(QTextDocument *parentObj)
	: QSyntaxHighlighter(parentObj),
	  highlightingRules(),
	  commentStartExpression(QRegExp("/\\*")),
	  commentEndExpression(QRegExp("\\*/")),
	  singleLineCommentFormat(),
	  multiLineCommentFormat(),
	  keywordFormat(),
	  classFormat(),
	  substitutionFormat(),
	  templateFormat()
{
	HighlightingRule rule;

	keywordFormat.setForeground(Qt::darkBlue);
	keywordFormat.setFontWeight(QFont::Bold);
	QStringList keywordPatterns;
	keywordPatterns << "\\bUPPERCASE" << "\\bCAPITALIZE" << "\\bIFEMPTY"
					<< "\\bBOOL" << "\\bRMLF" << "\\bFMT" << "\\b__TAB"
					<< "\\bHEX" << "\\bTREEMODE" << "\\b__DATE\\b"
					<< "\\b__UNIQUEID\\b" << "\\bLINECNT\\b"
					<< "\\bselect\\b" << "\\bfrom\\b" << "\\bwhere\\b"
					<< "\\border by\\b" << "\\bis\\b" << "\\bNULL\\b"
					<< "\\band\\b" << "\\bor\\b" << "\\bnot\\b";
	foreach (const QString &pattern, keywordPatterns)
	{
		rule.pattern = QRegExp(pattern);
		rule.pattern.setCaseSensitivity(Qt::CaseInsensitive);
		rule.format = keywordFormat;
		highlightingRules.append(rule);
	}

	classFormat.setFontWeight(QFont::Bold);
	classFormat.setForeground(Qt::darkMagenta);
	rule.pattern = QRegExp("^::.+");
	rule.format = classFormat;
	highlightingRules.append(rule);

	substitutionFormat.setForeground(Qt::red);
	rule.pattern = QRegExp("\\$\\{[^\\}]*\\}");
	rule.format = substitutionFormat;
	highlightingRules.append(rule);

	templateFormat.setForeground(Qt::darkMagenta);
	rule.pattern = QRegExp("\\#\\{[^\\}]*\\}");
	rule.format = templateFormat;
	highlightingRules.append(rule);

	singleLineCommentFormat.setForeground(Qt::green);
	rule.pattern = QRegExp("^//.*");
	rule.format = singleLineCommentFormat;
	highlightingRules.append(rule);

	multiLineCommentFormat.setForeground(Qt::green);
}

void SqlReportHighlighter::highlightBlock(const QString &text)
{
	foreach (const HighlightingRule &rule, highlightingRules)
	{
		QRegExp expression(rule.pattern);
		int index = expression.indexIn(text);
		while (index >= 0) {
			int length = expression.matchedLength();
			setFormat(index, length, rule.format);
			index = expression.indexIn(text, index + length);
		}
	}
	setCurrentBlockState(0);

	int startIndex = 0;
	if (previousBlockState() != 1)
		startIndex = commentStartExpression.indexIn(text);

	while (startIndex >= 0) {
		int endIndex = commentEndExpression.indexIn(text, startIndex);
		int commentLength;
		if (endIndex == -1) {
			setCurrentBlockState(1);
			commentLength = text.length() - startIndex;
		} else {
			commentLength = endIndex - startIndex
					+ commentEndExpression.matchedLength();
		}
		setFormat(startIndex, commentLength, multiLineCommentFormat);
		startIndex = commentStartExpression.indexIn(text, startIndex + commentLength);
	}
}