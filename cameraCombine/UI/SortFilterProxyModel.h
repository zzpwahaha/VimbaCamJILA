//Description: Filter pattern proxy

#ifndef SORTFILTERPROXYMODEL_H
#define SORTFILTERPROXYMODEL_H

#include <QSortFilterProxyModel>

class SortFilterProxyModel : public QSortFilterProxyModel
{
    public:
           SortFilterProxyModel(QObject *parent = 0);

    protected:
           bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const;

    private:
};

#endif
