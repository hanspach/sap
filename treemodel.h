#ifndef TREEMODEL_H
#define TREEMODEL_H

#include <QAbstractItemModel>
#include <QModelIndex>
#include <QVariant>
#include <QFileInfoList>
#include <QXmlStreamReader>
#include "treeitem.h"

class TreeModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    bool isNetMedia;

    explicit TreeModel(QObject *parent = 0);
    virtual ~TreeModel();

    QVariant data(const QModelIndex &index, int role) const;
    QVariant headerData(int section, Qt::Orientation orientation,
                     int role = Qt::DisplayRole) const;

    QModelIndex index(int row, int column,
                   const QModelIndex &parent = QModelIndex()) const;
    QModelIndex parent(const QModelIndex &index) const;

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;

    Qt::ItemFlags flags(const QModelIndex &index) const;
    bool setData(const QModelIndex &index, const QVariant &value,
              int role = Qt::EditRole);
    bool setHeaderData(int section, Qt::Orientation orientation,
                    const QVariant &value, int role = Qt::EditRole);

    bool insertColumns(int position, int columns,
                    const QModelIndex &parent = QModelIndex());
    bool removeColumns(int position, int columns,
                    const QModelIndex &parent = QModelIndex());
    bool insertRows(int position, int rows,
                 const QModelIndex &parent = QModelIndex());
    bool removeRows(int position, int rows,
                 const QModelIndex &parent = QModelIndex());
    TreeItem *getItem(const QModelIndex &index) const;

    void setupModelData(const QFileInfoList& filelist, const QModelIndex& parent=QModelIndex());
    void setupModelData(QFile& file, const QString &layer,
               const QString& parentName=QString(), const QModelIndex& parentIdx=QModelIndex());

    void clear();
protected:
    QString readAttribute(const QXmlStreamReader& reader, const QString& attrName);
    void clear(QModelIndex idx);
    TreeItem *rootItem;
};

#endif // TREEMODEL_H
