#include "treeitem.h"

TreeItem::TreeItem(const QVector<QVariant> &data, TreeItem *parent)
    : itemData(data)
{
    parentItem = parent;
}

TreeItem::TreeItem(const QString& name, TreeItem* parent)
{
    itemData << name;
    parentItem = parent;
}

TreeItem::~TreeItem()
{
    qDeleteAll(childItems);
}

TreeItem *TreeItem::child(int number)
{
    return childItems.value(number);
}

int TreeItem::childCount() const
{
    return childItems.count();
}

int TreeItem::childNumber() const
{
    if (parentItem)
        return parentItem->childItems.indexOf(const_cast<TreeItem*>(this));

    return 0;
}

int TreeItem::columnCount() const
{
    return itemData.count();
}

QVariant TreeItem::data(int column) const
{
    return itemData.value(column);
}

QString TreeItem::name() const
{
    return itemData.value(0).toString();
}

QString TreeItem::url() const
{
    return itemUrl;
}

QString TreeItem::iconPath() const
{
    return itemIconPath;
}

void TreeItem::setName(const QString &name)
{
    setData(0,name);
}

void TreeItem::setUrl(const QString &url)
{
    itemUrl = url;
}

void TreeItem::setIconPath(const QString &iconPath)
{
    itemIconPath = iconPath;
}

bool TreeItem::insertChildren(int position, int count, int columns)
{
     if (position < 0 || position > childItems.size())
         return false;

     for (int row = 0; row < count; ++row) {
         QVector<QVariant> data(columns);
         TreeItem *item = new TreeItem(data, this);
         childItems.insert(position, item);
     }

     return true;
}

bool TreeItem::insertColumns(int position, int columns)
{
     if (position < 0 || position > itemData.size())
         return false;

     for (int column = 0; column < columns; ++column)
         itemData.insert(position, QVariant());

     foreach (TreeItem *child, childItems)
         child->insertColumns(position, columns);

     return true;
}

TreeItem *TreeItem::parent()
{
     return parentItem;
}

bool TreeItem::removeChildren(int position, int count)
{
     if (position < 0 || position + count > childItems.size())
         return false;

     for (int row = 0; row < count; ++row)
         delete childItems.takeAt(position);

     return true;
}

bool TreeItem::removeColumns(int position, int columns)
{
     if (position < 0 || position + columns > itemData.size())
         return false;

     for (int column = 0; column < columns; ++column)
         itemData.remove(position);

     foreach (TreeItem *child, childItems)
         child->removeColumns(position, columns);

     return true;
}

bool TreeItem::setData(int column, const QVariant &value)
{
     if (column < 0 || column >= itemData.size())
         return false;

     itemData[column] = value;
     return true;
}
