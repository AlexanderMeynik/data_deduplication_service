#include "MyPqxxModel.h"


MyPqxxModel::MyPqxxModel(QObject *parent) : QAbstractTableModel(parent), MyPxxxModelBase() {
    res = pqxx::result();
    isEmpty_ = true;
}

QVariant MyPqxxModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (role == Qt::DisplayRole) {
        if (orientation == Qt::Orientation::Vertical) {
            return QVariant(section);
        }
        return QVariant(QString::fromStdString(res.column_name(section)));
    }
    return QVariant();
}

QVariant MyPqxxModel::data(const QModelIndex &index, int role) const {
    if (role == Qt::DisplayRole) {
        return getData(index,role);
    }
    return QVariant();
}





void MyPxxxModelBase::setColumnsTypes() {
    columnTypes.resize(res.columns());
    columnNames.resize(res.columns());
    for (int i = 0; i < res.columns(); ++i) {
        columnTypes[i] = res.column_type(i);
        columnNames[i] = res.column_name(i);
    }
}

bool MyPxxxModelBase::performConnection(myConnString &cstring) {
    this->connection_ = connectIfPossible(cstring.c_str()).value_or(nullptr);
    good = checkConnection();
    return good;
}
