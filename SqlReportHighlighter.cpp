#include "SqlReportHighlighter.h"
#include <QRegularExpression>

SqlReportHighlighter::SqlReportHighlighter(QTextDocument *parentObj)
	: QSyntaxHighlighter(parentObj),
	  highlightingRules(),
      commentStartExpression(QRegularExpression("/\\*")),
      commentEndExpression(QRegularExpression("\\*/")),
	  singleLineCommentFormat(),
	  singleBlockLineCommentFormat(),
	  multiLineCommentFormat(),
	  keywordFormat(),
	  blockFormat(),
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
					<< "\\band\\b" << "\\bor\\b" << "\\bnot\\b"
					<< "\\bunion\\b" << "\\blike\\b" << "\\bgroup by\\b"
					<< "\\bhaving\\b" << "\\bdistinct\\b" << "\\bin\\b"
					<< "\\bdistinct\\b" << "\\binner join\\b" << "\\bouter join\\b"
					<< "\\bleft join\\b" << "\\bright join\\b" << "\\bjoin\\b"
					<< "\\bcount\\b" << "\\bmin\\b" << "\\bmax\\b";
	foreach (const QString &pattern, keywordPatterns)
	{
        rule.pattern = QRegularExpression(pattern);
        rule.pattern.setPatternOptions(QRegularExpression::CaseInsensitiveOption);
		rule.format = keywordFormat;
		highlightingRules.append(rule);
	}

	blockFormat.setForeground(Qt::darkMagenta);
	blockFormat.setFontWeight(QFont::Bold);
    rule.pattern = QRegularExpression("^::.+");
	rule.format = blockFormat;
	highlightingRules.append(rule);

	substitutionFormat.setForeground(Qt::red);
    rule.pattern = QRegularExpression("\\$\\{[^\\}]*\\}");
	rule.format = substitutionFormat;
	highlightingRules.append(rule);

	templateFormat.setForeground(Qt::darkMagenta);
    rule.pattern = QRegularExpression("\\#\\{[^\\}]*\\}");
	rule.format = templateFormat;
	highlightingRules.append(rule);

	singleLineCommentFormat.setForeground(Qt::green);
    rule.pattern = QRegularExpression("^//.*");
	rule.format = singleLineCommentFormat;
	highlightingRules.append(rule);

	singleBlockLineCommentFormat.setForeground(Qt::darkGreen);
    rule.pattern = QRegularExpression("^::#.*");
	rule.format = singleBlockLineCommentFormat;
	highlightingRules.append(rule);

	multiLineCommentFormat.setForeground(Qt::green);
}

void SqlReportHighlighter::highlightBlock(const QString &text)
{
	foreach (const HighlightingRule &rule, highlightingRules)
	{
        QRegularExpression expression(rule.pattern);
        QRegularExpressionMatchIterator i = expression.globalMatch(text);
        while (i.hasNext())
        {
          QRegularExpressionMatch match = i.next();
          setFormat(match.capturedStart(), match.capturedLength(), rule.format);
        }
	}
//	setCurrentBlockState(0);

//	int startIndex = 0;
//	if (previousBlockState() != 1)
//		startIndex = commentStartExpression.indexIn(text);

//	while (startIndex >= 0) {
//		int endIndex = commentEndExpression.indexIn(text, startIndex);
//		int commentLength;
//		if (endIndex == -1) {
//			setCurrentBlockState(1);
//			commentLength = text.length() - startIndex;
//		} else {
//			commentLength = endIndex - startIndex
//					+ commentEndExpression.matchedLength();
//		}
//		setFormat(startIndex, commentLength, multiLineCommentFormat);
//		startIndex = commentStartExpression.indexIn(text, startIndex + commentLength);
//	}
}
