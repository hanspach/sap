#include <QtGui>
#include "treemodel.h"

TreeModel::TreeModel(QObject* parent) : QAbstractItemModel(parent)
{
    rootItem = new TreeItem("");
}

TreeModel::~TreeModel()
{
    delete rootItem;
}

int TreeModel::columnCount(const QModelIndex & /* parent */) const
{
    return rootItem->columnCount();
}

QVariant TreeModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    const TreeItem *item = getItem(index);
    if(role == Qt::DecorationRole && index.column()==0) {
        if(!item->iconPath().isNull() && !item->iconPath().isEmpty())
            return QIcon(item->iconPath());
    }
    if (role != Qt::DisplayRole && role != Qt::EditRole)
         return QVariant();

    return item->data(index.column());
}

 Qt::ItemFlags TreeModel::flags(const QModelIndex &index) const
 {
     if (!index.isValid())
         return 0;

     return Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemIsSelectable;
 }

 TreeItem *TreeModel::getItem(const QModelIndex &index) const
 {
     if (index.isValid()) {
         TreeItem *item = static_cast<TreeItem*>(index.internalPointer());
         if (item) return item;
     }
     return rootItem;
 }

 QVariant TreeModel::headerData(int section, Qt::Orientation orientation,
                                int role) const
 {
     if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
         return rootItem->data(section);

     return QVariant();
 }

 QModelIndex TreeModel::index(int row, int column, const QModelIndex &parent) const
 {
     if (parent.isValid() && parent.column() != 0)
         return QModelIndex();

     TreeItem *parentItem = getItem(parent);

     TreeItem *childItem = parentItem->child(row);
     if (childItem)
         return createIndex(row, column, childItem);
     else
         return QModelIndex();
 }

 bool TreeModel::insertColumns(int position, int columns, const QModelIndex &parent)
 {
     bool success;

     beginInsertColumns(parent, position, position + columns - 1);
     success = rootItem->insertColumns(position, columns);
     endInsertColumns();

     return success;
 }

bool TreeModel::insertRows(int position, int rows, const QModelIndex &parent)
{
     TreeItem *parentItem = getItem(parent);
     bool success;

     beginInsertRows(parent, position, position + rows - 1);
     success = parentItem->insertChildren(position, rows, rootItem->columnCount());
     endInsertRows();

     return success;
}

bool TreeModel::removeRows(int position, int rows, const QModelIndex &parent)
{
     TreeItem *parentItem = getItem(parent);
     bool success = true;

     beginRemoveRows(parent, position, position + rows - 1);
     success = parentItem->removeChildren(position, rows);
     endRemoveRows();

     return success;
}

QModelIndex TreeModel::parent(const QModelIndex &index) const
{
     if (!index.isValid())
         return QModelIndex();

     TreeItem *childItem = getItem(index);
     TreeItem *parentItem = childItem->parent();

     if (parentItem == rootItem)
         return QModelIndex();

     return createIndex(parentItem->childNumber(), 0, parentItem);
}

bool TreeModel::removeColumns(int position, int columns, const QModelIndex &parent)
{
     bool success;

     beginRemoveColumns(parent, position, position + columns - 1);
     success = rootItem->removeColumns(position, columns);
     endRemoveColumns();

     if (rootItem->columnCount() == 0)
         removeRows(0, rowCount());

     return success;
 }

int TreeModel::rowCount(const QModelIndex &parent) const
{
    TreeItem *parentItem = getItem(parent);
    return parentItem->childCount();
}

void TreeModel::clear(QModelIndex idx)
{
    for(int i=0; i < rowCount(idx); i++) {
        QModelIndex cdx = index(i,0,idx);
        clear(cdx);
        removeRows(i,1,idx);
    }
}

void TreeModel::clear()
{
    beginResetModel();
    int rows = rowCount();
    if(rows > 0) {
        removeRows(0,rows);
    }
    endResetModel();
}

bool TreeModel::setData(const QModelIndex &index, const QVariant &value,int role)
{
    if (role != Qt::EditRole)
         return false;

     TreeItem *item = getItem(index);
     bool result = item->setData(index.column(), value);

     if (result)
         emit dataChanged(index, index);

     return result;
}

 bool TreeModel::setHeaderData(int section, Qt::Orientation orientation,
                               const QVariant &value, int role)
 {
     if (role != Qt::EditRole || orientation != Qt::Horizontal)
         return false;

     bool result = rootItem->setData(section, value);

     if (result)
         emit headerDataChanged(orientation, section, section);

     return result;
 }

void TreeModel::setupModelData(const QFileInfoList& filelist, const QModelIndex &parent)
{
    int row = 0;
    foreach(QFileInfo info, filelist) {
       if(insertRows(row,1, parent)) {
           QModelIndex child = index(row, 0, parent);
           setData(child, info.baseName());
           TreeItem* item = getItem(child);
           if(item) {
               item->setUrl(info.absoluteFilePath());
               if(info.isDir()) {
                   item->setIconPath(":/resources/folder.png");
                   if(insertRows(0,1, child)) {
                       QModelIndex grandchild = index(0,0,child);
                       setData(grandchild, tr(""));
                   }
               }
               else
                   item->setIconPath(":/resources/audio.png");
           }
           ++row;
       }
    }
}

void TreeModel::setupModelData(QFile& file, const QString &layer, const QString& parentName, const QModelIndex& parentIdx)
{
    QXmlStreamReader reader;
    reader.setDevice(&file);
    int row = 0;
    bool found = false;

    while(!reader.atEnd()) {
        reader.readNext();
        if(reader.isStartElement()) {
            QString ebene= reader.name().toString();
            QString name = readAttribute(reader, tr("name"));
            QString url  = readAttribute(reader, tr("url"));

            if(!parentName.isEmpty()) {
                if(layer == "station") {
                    if(ebene=="country")
                        found = name == parentName;
                }
                else if(layer == "bookmark") {
                    if(ebene=="station")
                        found = name == parentName;
                }
            }
            else {
                found = true;
            }
            if(found) {
                if(ebene == layer) {
                    if(insertRows(row,1, parentIdx)) {
                        QModelIndex child = index(row, 0, parentIdx);
                        setData(child, name);
                        TreeItem* item = getItem(child);
                        if(item) {
                            item->setUrl(url);
                            if(ebene == "country")
                                item->setIconPath(":/resources/" + name.toLower() + ".png");
                            else if(ebene == "station")
                                item->setIconPath(":/resources/station.png");
                            else
                                item->setIconPath(":/resources/audio.png");
                        }
                        if(ebene == "country" || ebene == "station") {
                            if(insertRows(0,1, child)) {
                                QModelIndex grandchild = index(0,0,child);
                                setData(grandchild, tr(""));
                            }
                        }
                        ++row;
                    }
                }
            }
        }
    }
}

QString TreeModel::readAttribute(const QXmlStreamReader& reader, const QString& attrName)
{
    QString res = QString();
    if(reader.attributes().hasAttribute(attrName))
        res += reader.attributes().value(attrName);
    return res;
}
