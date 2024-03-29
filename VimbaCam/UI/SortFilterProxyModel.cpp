//Description: Filter pattern proxy

#include "SortFilterProxyModel.h"


SortFilterProxyModel::SortFilterProxyModel(QObject *parent)
    : QSortFilterProxyModel(parent)
{

}

bool SortFilterProxyModel::filterAcceptsRow(int source_row, const QModelIndex & source_parent) const
{
    // get source-model index for current row
    QModelIndex source_index = sourceModel()->index(source_row, this->filterKeyColumn(), source_parent) ;
    if(source_index.isValid())
    {
        // if any of children matches the filter, then current index matches the filter as well
        int nCount = sourceModel()->rowCount(source_index) ;
        for(int i=0; i<nCount; ++i)
        {
            // this is a recursive iteration untill it reaches the last layer
            if(filterAcceptsRow(i, source_index))
            {
                return true ;
            }
        }
        // check current index itself :
        QString key = sourceModel()->data(source_index, filterRole()).toString();
        return key.contains(filterRegExp()) ;
    }
    
    // parent call for initial behaviour
    return QSortFilterProxyModel::filterAcceptsRow(source_row, source_parent) ;
}

