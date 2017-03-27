#ifndef TREEITEM_H
#define TREEITEM_H
#include <QList>
#include <QVariant>
#include <QVector>

class TreeItem
{
public:
    TreeItem(const QVector<QVariant> &data, TreeItem *parent = 0);
    TreeItem(const QString& name, TreeItem* parent = 0);
    ~TreeItem();

    TreeItem *child(int number);
    int childCount() const;
    int columnCount() const;
    QVariant data(int column) const;
    QString name() const;
    void setName(const QString& name);
    QString url() const;
    void setUrl(const QString& url);
    QString iconPath() const;
    void setIconPath(const QString& iconPath);

    bool insertChildren(int position, int count, int columns);
    bool insertColumns(int position, int columns);
    TreeItem *parent();
    bool removeChildren(int position, int count);
    bool removeColumns(int position, int columns);
    int childNumber() const;
    bool setData(int column, const QVariant &value);
private:
    QList<TreeItem*> childItems;
    QVector<QVariant> itemData;
    TreeItem *parentItem;
    QString itemUrl;
    QString itemIconPath;
};

#endif // TREEITEM_H
