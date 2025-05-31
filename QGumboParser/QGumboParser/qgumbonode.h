#ifndef QGUMBONODE_H
#define QGUMBONODE_H

#include <vector>
#include <functional>
#include "gumbo-parser/src/gumbo.h"
#include "HtmlTag.h"

class QString;
class QGumboNode;
class QGumboAttribute;
class QGumboDocument;
class QStringList;

typedef std::vector<QGumboNode> 		QGumboNodes;
typedef std::vector<QGumboAttribute> 	QGumboAttributes;

class QGumboNode
{
public:
    QGumboNode(const QGumboNode&) = default;
    QGumboNode(QGumboNode&&) noexcept = default;
    QGumboNode& operator=(const QGumboNode&) = default;

    HtmlTag tag() const;
    QString tagName() const;
    QString nodeName() const;

    QString id() const;
    QStringList classList() const;

    QGumboNodes getElementById(const QString&) const;
    QGumboNodes getElementsByTagName(HtmlTag) const;
    QGumboNodes getElementsByClassName(const QString&) const;
    QGumboNodes childNodes() const;
    QGumboNodes children() const;

    int childElementCount() const;

    bool isElement() const;
    bool hasAttribute(const QString&) const;

    // 获取一级子节点内的文本
    QString innerText() const;

    // 获取子节点内的所有文本
    QString innerTextV2() const;

    QString outerHtml() const;
    QString getAttribute(const QString&) const;

    QGumboAttributes allAttributes() const;

    void forEach(std::function<void(const QGumboNode&)>) const;

    explicit operator bool() const;

private:
    QGumboNode();
    QGumboNode(GumboNode* node);

    friend class QGumboDocument;
private:
    GumboNode* ptr_;
};

#endif // QGUMBONODE_H
